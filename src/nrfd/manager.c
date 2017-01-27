/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <glib.h>
#include <gio/gio.h>
#include <json-c/json.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "include/nrf24.h"
#include "include/comm.h"
#include "include/time.h"

#include "nrf24l01_io.h"
#include "log.h"
#include "manager.h"

#define KNOTD_UNIX_ADDRESS		"knot"
#define MAC_ADDRESS_SIZE		24

static int mgmtfd;
static guint mgmtwatch;
static guint dbus_id;

static struct adapter {
	struct nrf24_mac mac;
	gboolean powered;
	/* Struct with the known peers */
	struct {
		struct nrf24_mac addr;
		guint registration_id;
		gchar *alias;
		gboolean status;
	} known_peers[MAX_PEERS];
	guint known_peers_size;
} adapter;

struct peer {
	char name[10];
	uint64_t mac;
	int8_t socket_fd;
	int8_t knotd_fd;
	GIOChannel *knotd_io;
	guint knotd_id;
};

static struct peer peers[MAX_PEERS] = {
	{.socket_fd = -1},
	{.socket_fd = -1},
	{.socket_fd = -1},
	{.socket_fd = -1},
	{.socket_fd = -1}
};

static uint8_t count_clients;

static GDBusNodeInfo *introspection_data = NULL;

/* Introspection data for the service we are exporting */
static const gchar introspection_xml[] =
	"<node>"
	"  <interface name='org.cesar.knot.nrf0.Adapter'>"
	"    <method name='AddDevice'>"
	"      <arg type='s' name='mac' direction='in'/>"
	"      <arg type='s' name='key' direction='in'/>"
	"      <arg type='b' name='response' direction='out'/>"
	"    </method>"
	"    <method name='RemoveDevice'>"
	"      <arg type='s' name='mac' direction='in'/>"
	"      <arg type='b' name='response' direction='out'/>"
	"    </method>"
	"    <property type='s' name='Address' access='read'/>"
	"    <property type='s' name='Powered' access='readwrite'/>"
	"  </interface>"
	"</node>";

static GVariant *handle_device_get_property(GDBusConnection *connection,
				const gchar *sender,
				const gchar *object_path,
				const gchar *interface_name,
				const gchar *property_name,
				GError **error,
				gpointer user_data)
{
	GVariant *gvar = NULL;
	char str_mac[24];
	gint dev;

	dev = GPOINTER_TO_INT(user_data);
	/* TODO implement Alias and Status */
	if (g_strcmp0(property_name, "Mac") == 0) {
		nrf24_mac2str(&adapter.known_peers[dev].addr, str_mac);
		gvar = g_variant_new("s", str_mac);
	} else if (g_strcmp0(property_name, "Alias") == 0) {
		gvar = g_variant_new("s", adapter.known_peers[dev].alias);
	} else if (g_strcmp0(property_name, "Status") == 0) {
		gvar = g_variant_new_boolean(adapter.known_peers[dev].status);
	} else {
		gvar = g_variant_new_string("Unknown property requested");
	}

	return gvar;
}

static int8_t new_device_object(GDBusConnection *connection, uint32_t free_pos)
{
	uint8_t i;
	guint registration_id;
	GDBusInterfaceInfo *interface;
	GDBusPropertyInfo **properties;
	gchar object_path[26];
	gchar *name[] = {"Mac", "Alias", "Status"};
	gchar *signature[] = {"s", "s", "b"};
	GDBusInterfaceVTable interface_device_vtable = {
		NULL,
		handle_device_get_property,
		NULL
	};

	properties = g_new0(GDBusPropertyInfo *, 4);
	for (i = 0; i < 3; i++) {
		properties[i] = g_new0(GDBusPropertyInfo, 1);
		/*
		 * properties->ref_count is incremented here because when
		 * registering an object the function only increments
		 * interface->ref_count. Not doing this leads to the memory
		 * of properties not being deallocated when we call
		 * g_dbus_connection_unregister_object.
		 */
		g_atomic_int_inc(&properties[i]->ref_count);
		properties[i]->name = g_strdup(name[i]);
		properties[i]->signature = g_strdup(signature[i]);
		properties[i]->flags = G_DBUS_PROPERTY_INFO_FLAGS_READABLE;
		properties[i]->annotations = NULL;
	}

	interface = g_new0(GDBusInterfaceInfo, 1);
	interface->name = g_strdup("org.cesar.knot.nrf.Device");
	interface->methods = NULL;
	interface->signals = NULL;
	interface->properties = properties;
	interface->annotations = NULL;

	snprintf(object_path, 26, "/org/cesar/knot/nrf0/mac%d", free_pos);

	registration_id = g_dbus_connection_register_object(
					connection,
					object_path,
					interface,
					&interface_device_vtable,
					GINT_TO_POINTER(free_pos),  /* user data */
					NULL,  /* user data free func */
					NULL); /* GError** */
	/* Free mem if fail */
	if (registration_id <= 0) {
		for (i = 0; properties[i] != NULL; i++) {
			g_free(properties[i]->name);
			g_free(properties[i]->signature);
			g_free(properties[i]);
		}
		g_free(properties);
		g_free(interface->name);
		g_free(interface);
		return -1;
	}
	adapter.known_peers[free_pos].registration_id = registration_id;
	return 0;
}

static gboolean add_known_device(GDBusConnection *connection, const gchar *mac,
							const gchar *key)
{
	uint8_t alloc_pos, i;
	int32_t free_pos;
	gchar alias[7];
	gboolean response = FALSE;
	struct nrf24_mac new_dev;

	if (nrf24_str2mac(mac, &new_dev) < 0)
		goto done;
	/* TODO: update keys file whith any change to struct*/
	for (i = 0, alloc_pos = 0, free_pos = -1; alloc_pos <
					adapter.known_peers_size; i++) {
		if (adapter.known_peers[i].addr.address.uint64 ==
						new_dev.address.uint64) {
			/* TODO: Update key of existing mac */
			response = TRUE;
			goto done;

		} else if (adapter.known_peers[i].addr.address.uint64 != 0) {
			alloc_pos++;
		} else if (free_pos < 0) {
			/* store available position in array */
			free_pos = i;
		}
	}
	/* If there is no empty space between allocated spaces */
	if (free_pos < 0)
		free_pos = i;

	/* If struct has free space add device to struct */
	if (adapter.known_peers_size < MAX_PEERS) {
		if (new_device_object(connection, free_pos) < 0)
			goto done;

		adapter.known_peers[free_pos].addr.address.uint64 =
							new_dev.address.uint64;
		snprintf(alias, 7, "Thing%d", free_pos);
		adapter.known_peers[free_pos].alias = g_strdup(alias);
		adapter.known_peers[free_pos].status = FALSE;
		/* TODO: Set key for this mac */
		adapter.known_peers_size++;
		response = TRUE;
	}

done:
	return response;
}

static gboolean remove_known_device(GDBusConnection *connection,
							const gchar *mac)
{
	uint8_t i;
	gboolean response = FALSE;
	struct nrf24_mac dev;

	if (nrf24_str2mac(mac, &dev) < 0)
		return FALSE;
	/* TODO: update keys file to remove mac */
	for (i = 0; i < MAX_PEERS; i++) {
		if (adapter.known_peers[i].addr.address.uint64 ==
							dev.address.uint64) {
			if (!g_dbus_connection_unregister_object(connection,
				adapter.known_peers[i].registration_id))
				break;
			/* Remove mac from struct */
			adapter.known_peers[i].addr.address.uint64 = 0;
			g_free(adapter.known_peers[i].alias);
			adapter.known_peers_size--;
			response = TRUE;
			break;
		}
	}

	return response;
}

static void handle_method_call(GDBusConnection *connection,
				const gchar *sender,
				const gchar *object_path,
				const gchar *interface_name,
				const gchar *method_name,
				GVariant *parameters,
				GDBusMethodInvocation *invocation,
				gpointer user_data)
{
	const gchar *mac;
	const gchar *key;
	gboolean response;

	if (g_strcmp0(method_name, "AddDevice") == 0) {
		g_variant_get(parameters, "(&s&s)", &mac, &key);
		/* Add or Update mac address */
		response = add_known_device(connection, mac, key);
		g_dbus_method_invocation_return_value(invocation,
				g_variant_new("(b)", response));
	} else if (g_strcmp0(method_name, "RemoveDevice") == 0) {
		g_variant_get(parameters, "(&s)", &mac);
		/* Remove device */
		response = remove_known_device(connection, mac);
		g_dbus_method_invocation_return_value(invocation,
				g_variant_new("(b)", response));
	}
}

static GVariant *handle_get_property(GDBusConnection  *connection,
				const gchar *sender,
				const gchar *object_path,
				const gchar *interface_name,
				const gchar *property_name,
				GError **gerr,
				gpointer user_data)
{
	GVariant *gvar = NULL;
	char str_mac[24];

	if (g_strcmp0(property_name, "Address") == 0) {
		nrf24_mac2str(&adapter.mac, str_mac);
		gvar = g_variant_new("s", str_mac);
	} else if (g_strcmp0(property_name, "Powered") == 0) {
		gvar = g_variant_new_boolean(adapter.powered);
	} else {
		gvar = g_variant_new_string("Unknown property requested");
	}

	return gvar;
}

static gboolean handle_set_property(GDBusConnection  *connection,
				const gchar *sender,
				const gchar *object_path,
				const gchar *interface_name,
				const gchar *property_name,
				GVariant *value,
				GError **gerr,
				gpointer user_data)
{
	if (g_strcmp0(property_name, "Powered") == 0) {
		adapter.powered = g_variant_get_boolean(value);
		/* TODO turn adapter on or off */
	}
	return TRUE;
}

static const GDBusInterfaceVTable interface_vtable = {
	handle_method_call,
	handle_get_property,
	handle_set_property
};

static void on_bus_acquired(GDBusConnection *connection, const gchar *name,
							gpointer user_data)
{
	uint8_t i, j, k;
	guint registration_id;
	GDBusInterfaceInfo *interface;
	GDBusPropertyInfo **properties;
	gchar object_path[26];
	gchar *obj_name[] = {"Mac", "Alias", "Status"};
	gchar *signature[] = {"s", "s", "b"};
	GDBusInterfaceVTable interface_device_vtable = {
		NULL,
		handle_device_get_property,
		NULL
	};

	registration_id = g_dbus_connection_register_object(connection,
					"/org/cesar/knot/nrf0",
					introspection_data->interfaces[0],
					&interface_vtable,
					NULL,  /* user_data */
					NULL,  /* user_data_free_func */
					NULL); /* GError** */
	g_assert(registration_id > 0);

	/* Register on dbus every device already known */
	for (j = 0, k = 0; j < adapter.known_peers_size; j++, k++) {
		properties = g_new0(GDBusPropertyInfo *, 4);
		for (i = 0; i < 3; i++) {
			properties[i] = g_new0(GDBusPropertyInfo, 1);
			/*
			 * properties->ref_count is incremented here because
			 * when registering an object the function only
			 * increments interface->ref_count. Not doing this leads
			 * to the memory of properties not being deallocated
			 * when we call g_dbus_connection_unregister_object.
			 */
			g_atomic_int_inc(&properties[i]->ref_count);
			properties[i]->name = g_strdup(obj_name[i]);
			properties[i]->signature = g_strdup(signature[i]);
			properties[i]->flags =
					G_DBUS_PROPERTY_INFO_FLAGS_READABLE;
			properties[i]->annotations = NULL;
		}

		interface = g_new0(GDBusInterfaceInfo, 1);
		interface->name = g_strdup("org.cesar.knot.mac.Device");
		interface->methods = NULL;
		interface->signals = NULL;
		interface->properties = properties;
		interface->annotations = NULL;

		snprintf(object_path, 26, "/org/cesar/knot/nrf0/mac%d", k);

		registration_id = g_dbus_connection_register_object(
						connection,
						object_path,
						interface,
						&interface_device_vtable,
						GINT_TO_POINTER(k),
						NULL,  /* user data free func */
						NULL); /* GError** */
		if (registration_id <= 0) {
			for (i = 0; properties[i] != NULL; i++) {
				g_free(properties[i]->name);
				g_free(properties[i]->signature);
				g_free(properties[i]);
			}
			g_free(properties);
			g_free(interface->name);
			g_free(interface);
			k--;
			continue;
		}
		adapter.known_peers[k].registration_id = registration_id;
	}
}

static void on_name_acquired(GDBusConnection *connection, const gchar *name,
							gpointer user_data)
{
	/* Connection successfully estabilished */
	log_info("Connection estabilished");
}

static void on_name_lost(GDBusConnection *connection, const gchar *name,
							gpointer user_data)
{
	if (!connection) {
		/* Connection error */
		log_error("Connection failure");
	} else {
		/* Name not owned */
		log_error("Name can't be obtained");
	}

	g_dbus_node_info_unref(introspection_data);
	exit(EXIT_FAILURE);
}

static guint dbus_init(struct nrf24_mac mac)
{
	guint owner_id;

	introspection_data = g_dbus_node_info_new_for_xml(introspection_xml,
									NULL);
	g_assert(introspection_data != NULL);

	owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM,
					"org.cesar.knot.nrf",
					G_BUS_NAME_OWNER_FLAGS_NONE,
					on_bus_acquired, on_name_acquired,
					on_name_lost, NULL, NULL);
	adapter.mac = mac;
	adapter.powered = TRUE;

	return owner_id;
}

static void dbus_on_close(guint owner_id)
{
	uint8_t i;

	for (i = 0; i < MAX_PEERS; i++) {
		if (adapter.known_peers[i].addr.address.uint64 != 0)
			g_free(adapter.known_peers[i].alias);
	}
	g_bus_unown_name(owner_id);
	g_dbus_node_info_unref(introspection_data);
}

/* Check if peer is on list of known peers */
static int8_t check_permission(struct nrf24_mac mac)
{
	uint8_t i;

	for (i = 0; i < MAX_PEERS; i++) {
		if (mac.address.uint64 ==
				adapter.known_peers[i].addr.address.uint64)
			return 0;
	}

	return -EPERM;
}

/* Get peer position in vector of peers*/
static int8_t get_peer(struct nrf24_mac mac)
{
	int8_t i;

	for (i = 0; i < MAX_PEERS; i++)
		if (peers[i].socket_fd != -1 &&
			peers[i].mac == mac.address.uint64)
			return i;

	return -EINVAL;
}

/* Get free position in vector for peers*/
static int8_t get_peer_index(void)
{
	int8_t i;

	for (i = 0; i < MAX_PEERS; i++)
		if (peers[i].socket_fd == -1)
			return i;

	return -EUSERS;
}

static int connect_unix(void)
{
	struct sockaddr_un addr;
	int sock;

	sock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
	if (sock < 0)
		return -errno;

	/* Represents unix socket from nrfd to knotd */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path + 1, KNOTD_UNIX_ADDRESS,
					strlen(KNOTD_UNIX_ADDRESS));

	if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		return -errno;

	return sock;
}

static void knotd_io_destroy(gpointer user_data)
{
	struct peer *p = (struct peer *)user_data;
	hal_comm_close(p->socket_fd);
	close(p->knotd_fd);
	p->socket_fd = -1;
	p->knotd_id = 0;
	p->knotd_io = NULL;
	count_clients--;
}

static gboolean knotd_io_watch(GIOChannel *io, GIOCondition cond,
							gpointer user_data)
{

	char buffer[128];
	ssize_t readbytes_knotd;
	struct peer *p = (struct peer *)user_data;

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
		return FALSE;

	/* Read data from Knotd */
	readbytes_knotd = read(p->knotd_fd, buffer, sizeof(buffer));
	if (readbytes_knotd < 0) {
		log_error("read_knotd() error");
		return FALSE;
	}

	/* Send data to thing */
	/* TODO: put data in list for transmission */
	hal_comm_write(p->socket_fd, buffer, readbytes_knotd);

	return TRUE;
}

static int8_t evt_presence(struct mgmt_nrf24_header *mhdr)
{
	GIOCondition cond = G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
	int8_t position;
	uint8_t i;
	int err;
	char mac_str[MAC_ADDRESS_SIZE];
	struct mgmt_evt_nrf24_bcast_presence *evt_pre =
			(struct mgmt_evt_nrf24_bcast_presence *) mhdr->payload;

	/*
	 * Print every MAC sending presence in order to ease the discover of
	 * things trying to connect to the gw.
	 */
	nrf24_mac2str(&evt_pre->mac, mac_str);
	log_info("Thing sending presence. MAC = %s Name =%s",
			mac_str, evt_pre->name);

	/* Check if peer is allowed to connect */
	if (check_permission(evt_pre->mac) < 0)
		return -EPERM;

	if (count_clients >= MAX_PEERS)
		return -EUSERS; /*MAX PEERS*/

	/*Check if this peer is already allocated */
	position = get_peer(evt_pre->mac);
	/* If this is a new peer */
	if (position < 0) {
		/* Get free peers position */
		position = get_peer_index();
		if (position < 0)
			return position;

		/*Create Socket */
		err = hal_comm_socket(HAL_COMM_PF_NRF24, HAL_COMM_PROTO_RAW);
		if (err < 0)
			return err;

		peers[position].socket_fd = err;

		peers[position].knotd_fd = connect_unix();
		if (peers[position].knotd_fd < 0) {
			hal_comm_close(peers[position].socket_fd);
			peers[position].socket_fd = -1;
			return peers[position].knotd_fd;
		}

		/* Set mac value for this position */
		peers[position].mac =
				evt_pre->mac.address.uint64;

		/* Copy the slave name */
		memcpy(peers[position].name, evt_pre->name,
					sizeof(peers[position].name));
		/* Watch knotd socket */
		peers[position].knotd_io =
			g_io_channel_unix_new(peers[position].knotd_fd);
		g_io_channel_set_flags(peers[position].knotd_io,
			G_IO_FLAG_NONBLOCK, NULL);
		g_io_channel_set_close_on_unref(peers[position].knotd_io,
			FALSE);

		peers[position].knotd_id =
			g_io_add_watch_full(peers[position].knotd_io,
						G_PRIORITY_DEFAULT,
						cond,
						knotd_io_watch,
						&peers[position],
						knotd_io_destroy);
		g_io_channel_unref(peers[position].knotd_io);

		count_clients++;

		for (i = 0; i < MAX_PEERS; i++) {
			if (evt_pre->mac.address.uint64 ==
				adapter.known_peers[i].addr.address.uint64) {
				adapter.known_peers[i].status = TRUE;
				break;
			}
		}
	}

	/*Send Connect */
	hal_comm_connect(peers[position].socket_fd,
			&evt_pre->mac.address.uint64);
	return 0;
}

static int8_t evt_disconnected(struct mgmt_nrf24_header *mhdr)
{

	int8_t position;

	struct mgmt_evt_nrf24_disconnected *evt_disc =
			(struct mgmt_evt_nrf24_disconnected *) mhdr->payload;

	if (count_clients == 0)
		return -EINVAL;

	position = get_peer(evt_disc->mac);
	if (position < 0)
		return position;

	g_source_remove(peers[position].knotd_id);
	return 0;
}

/* Read RAW from Clients */
static int8_t clients_read()
{
	int8_t i;
	uint8_t buffer[256];
	int ret;

	/*No client */
	if (count_clients == 0)
		return 0;

	for (i = 0; i < MAX_PEERS; i++) {
		if (peers[i].socket_fd == -1)
			continue;

		ret = hal_comm_read(peers[i].socket_fd, &buffer,
			sizeof(buffer));
		if (ret > 0) {
			if (write(peers[i].knotd_fd, buffer, ret) < 0)
				log_error("write_knotd() error");
		}
	}
	return 0;
}

static int8_t mgmt_read(void)
{

	uint8_t buffer[256];
	struct mgmt_nrf24_header *mhdr = (struct mgmt_nrf24_header *) buffer;
	ssize_t rbytes;

	rbytes = hal_comm_read(mgmtfd, buffer, sizeof(buffer));

	/* mgmt on bad state? */
	if (rbytes < 0 && rbytes != -EAGAIN)
		return -1;

	/* Nothing to read? */
	if (rbytes == -EAGAIN)
		return -1;

	/* Return/ignore if it is not an event? */
	if (!(mhdr->opcode & 0x0200))
		return -1;

	switch (mhdr->opcode) {

	case MGMT_EVT_NRF24_BCAST_PRESENCE:
		evt_presence(mhdr);
		break;

	case MGMT_EVT_NRF24_BCAST_SETUP:
		break;

	case MGMT_EVT_NRF24_BCAST_BEACON:
		break;

	case MGMT_EVT_NRF24_DISCONNECTED:
		evt_disconnected(mhdr);
		break;
	}
	return 0;
}

static gboolean read_idle(gpointer user_data)
{
	mgmt_read();
	clients_read();
	return TRUE;
}

static int radio_init(const char *spi, uint8_t channel, uint8_t rfpwr,
						const struct nrf24_mac *mac)
{
	int err;

	err = hal_comm_init("NRF0", mac);
	if (err < 0)
		return err;

	mgmtfd = hal_comm_socket(HAL_COMM_PF_NRF24, HAL_COMM_PROTO_MGMT);
	if (mgmtfd < 0)
		goto done;

	mgmtwatch = g_idle_add(read_idle, NULL);

	return 0;
done:
	hal_comm_deinit();

	return mgmtfd;
}

static void close_clients(void)
{
	int i;

	for (i = 0; i < MAX_PEERS; i++) {
		if (peers[i].socket_fd != -1)
			g_source_remove(peers[i].knotd_id);
	}
}

static void radio_stop(void)
{
	close_clients();
	hal_comm_close(mgmtfd);
	if (mgmtwatch)
		g_source_remove(mgmtwatch);
	hal_comm_deinit();
}

static gboolean nrf_data_watch(GIOChannel *io, GIOCondition cond,
						gpointer user_data)
{
	char buffer[1024];
	GIOStatus status;
	GError *gerr = NULL;
	gsize rbytes;

	/*
	 * Manages TCP data from spiproxyd(nRF proxy). All traffic(raw
	 * data) should be transferred using unix socket to knotd.
	 */

	if (cond & (G_IO_HUP | G_IO_ERR))
		return FALSE;

	memset(buffer, 0, sizeof(buffer));

	/* Incoming data through TCP socket */
	status = g_io_channel_read_chars(io, buffer, sizeof(buffer),
						 &rbytes, &gerr);
	if (status == G_IO_STATUS_ERROR) {
		log_error("read(): %s", gerr->message);
		g_error_free(gerr);
		return FALSE;
	}

	if (rbytes == 0)
		return FALSE;

	/*
	 * Decode based on nRF PIPE information and forward
	 * the data through a unix socket to knotd.
	 */
	log_info("read(): %zu bytes", rbytes);

	return TRUE;
}

static int tcp_init(const char *host, int port)
{
	GIOChannel *io;
	GIOCondition cond = G_IO_IN | G_IO_ERR | G_IO_HUP;
	struct hostent *hostent;		/* Host information */
	struct in_addr h_addr;			/* Internet address */
	struct sockaddr_in server;		/* nRF proxy: spiproxyd */
	int err, sock;

	hostent = gethostbyname(host);
	if (hostent == NULL) {
		err = errno;
		log_error("gethostbyname(): %s(%d)", strerror(err), err);
		return -err;
	}

	h_addr.s_addr = *((unsigned long *) hostent-> h_addr_list[0]);

	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		err = errno;
		log_error("socket(): %s(%d)", strerror(err), err);
		return -err;
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = h_addr.s_addr;
	server.sin_port = htons(port);

	err = connect(sock, (struct sockaddr *) &server, sizeof(server));
	if (err < 0) {
		err = errno;
		log_error("connect(): %s(%d)", strerror(err), err);
		close(sock);
		return -err;
	}

	log_info("nRF Proxy address: %s", inet_ntoa(h_addr));

	io = g_io_channel_unix_new(sock);
	g_io_channel_set_close_on_unref(io, TRUE);

	/* Ending 'NULL' for binary data */
	g_io_channel_set_encoding(io, NULL, NULL);
	g_io_channel_set_buffered(io, FALSE);

	/* TCP handler: incoming data from spiproxyd (nRF proxy) */
	g_io_add_watch(io, cond, nrf_data_watch, NULL);

	/* Keep only one reference: watch */
	g_io_channel_unref(io);

	return 0;
}

static char *load_config(const char *file)
{
	char *buffer = NULL;
	int length;
	FILE *fl = fopen(file, "r");

	if (fl == NULL) {
		log_error("No such file available: %s", file);
		return NULL;
	}

	fseek(fl, 0, SEEK_END);
	length = ftell(fl);
	fseek(fl, 0, SEEK_SET);

	buffer = (char *) malloc((length+1)*sizeof(char));
	if (buffer) {
		fread(buffer, length, 1, fl);
		buffer[length] = '\0';
	}
	fclose(fl);

	return buffer;
}

/* Set TX Power from dBm to values defined at nRF24 datasheet */
static uint8_t dbm_int2rfpwr(int dbm)
{
	switch (dbm) {

	case 0:
		return NRF24_PWR_0DBM;

	case -6:
		return NRF24_PWR_6DBM;

	case -12:
		return NRF24_PWR_12DBM;

	case -18:
		return NRF24_PWR_18DBM;
	}

	/* Return default value when dBm value is invalid */
	return NRF24_PWR_0DBM;
}

static int gen_save_mac(const char *config, const char *file,
							struct nrf24_mac *mac)
{
	json_object *jobj, *obj_radio, *obj_tmp;

	int err = -EINVAL;

	jobj = json_tokener_parse(config);
	if (jobj == NULL)
		return -EINVAL;

	if (!json_object_object_get_ex(jobj, "radio", &obj_radio))
		goto done;

	if (json_object_object_get_ex(obj_radio,  "mac", &obj_tmp)){

			char mac_string[24];
			uint8_t mac_mask = 4;
			mac->address.uint64 = 0;

			hal_getrandom(mac->address.b + mac_mask,
						sizeof(*mac) - mac_mask);

			err = nrf24_mac2str((const struct nrf24_mac *) mac,
								mac_string);
			if (err == -1)
				goto done;

			json_object_object_add(obj_radio, "mac",
					json_object_new_string(mac_string));

			json_object_to_file((char *) file, jobj);
	}

	/* Success */
	err = 0;

done:
	/* Free mem used in json parse: */
	json_object_put(jobj);
	return err;
}

/*
 * TODO: Get "host", "spi" and "port"
 * parameters when/if implemented
 * in the json configuration file
 */
static int parse_config(const char *config, int *channel, int *dbm,
							struct nrf24_mac *mac)
{
	json_object *jobj, *obj_radio, *obj_tmp;

	int err = -EINVAL;

	jobj = json_tokener_parse(config);
	if (jobj == NULL)
		return -EINVAL;

	if (!json_object_object_get_ex(jobj, "radio", &obj_radio))
		goto done;

	if (json_object_object_get_ex(obj_radio, "channel", &obj_tmp))
		*channel = json_object_get_int(obj_tmp);

	if (json_object_object_get_ex(obj_radio,  "TxPower", &obj_tmp))
		*dbm = json_object_get_int(obj_tmp);

	if (json_object_object_get_ex(obj_radio,  "mac", &obj_tmp))
		if (json_object_get_string(obj_tmp) != NULL){
			err =
			nrf24_str2mac(json_object_get_string(obj_tmp), mac);
			if (err == -1)
				goto done;
		}

	/* Success */
	err = 0;

done:
	/* Free mem used in json parse: */
	json_object_put(jobj);
	return err;
}

static int parse_nodes(const char *nodes_file)
{
	int array_len;
	int i;
	int err = -EINVAL;
	json_object *jobj;
	json_object *obj_keys, *obj_nodes, *obj_tmp;

	/* Load nodes' info from json file */
	jobj = json_object_from_file(nodes_file);
	if (!jobj)
		return -EINVAL;

	if (!json_object_object_get_ex(jobj, "keys", &obj_keys))
		goto failure;

	array_len = json_object_array_length(obj_keys);
	if (array_len > MAX_PEERS) {
		log_error("Invalid numbers of nodes in input archive");
		goto failure;
	}
	for (i = 0; i < array_len; i++) {
		obj_nodes = json_object_array_get_idx(obj_keys, i);
		if (!json_object_object_get_ex(obj_nodes, "mac", &obj_tmp))
			goto failure;

		/* Parse mac address string into struct nrf24_mac known_peers */
		if (nrf24_str2mac(json_object_get_string(obj_tmp),
					&adapter.known_peers[i].addr) < 0)
			goto failure;
		adapter.known_peers_size++;

		if (!json_object_object_get_ex(obj_nodes, "name", &obj_tmp))
			goto failure;

		/* Set the name of the peer registered */
		adapter.known_peers[i].alias =
				g_strdup(json_object_get_string(obj_tmp));
		adapter.known_peers[i].status = FALSE;
	}

	err = 0;
failure:
	/* Free mem used to parse json */
	json_object_put(jobj);
	return err;
}

int manager_start(const char *file, const char *host, int port,
					const char *spi, int channel, int dbm,
					const char *nodes_file)
{
	int cfg_channel = NRF24_CH_MIN, cfg_dbm = 0;
	char *json_str;
	struct nrf24_mac mac = {.address.uint64 = 0};
	int err = -1;

	/* Command line arguments have higher priority */
	json_str = load_config(file);
	if (json_str == NULL)
		return err;
	err = parse_config(json_str, &cfg_channel, &cfg_dbm, &mac);
	if (err < 0)
		return err;

	memset(&adapter, 0, sizeof(struct adapter));
	/* Parse nodes info from nodes_file and writes it to known_peers */
	err = parse_nodes(nodes_file);
	if (err < 0)
		return err;

	if (mac.address.uint64 == 0)
		err = gen_save_mac(json_str, file, &mac);

	free(json_str);
	adapter.mac = mac;
	adapter.powered = TRUE;

	if (err < 0) {
		log_error("Invalid configuration file: %s", file);
		return err;
	}
	/* Start server dbus */
	dbus_id = dbus_init(mac);

	 /* Validate and set the channel */
	if (channel < 0 || channel > 125)
		channel = cfg_channel;

	/*
	 * Use TX Power from configuration file if it has not been passed
	 * through cmd line. -255 means invalid: not informed by user.
	 */
	if (dbm == -255)
		dbm = cfg_dbm;

	if (host == NULL)
		return radio_init(spi, channel, dbm_int2rfpwr(dbm),
						(const struct nrf24_mac*) &mac);
	/*
	 * TCP development mode: Linux connected to RPi(phynrfd radio
	 * proxy). Connect to phynrfd routing all traffic over TCP.
	 */
	return tcp_init(host, port);
}

void manager_stop(void)
{
	dbus_on_close(dbus_id);
	radio_stop();
}

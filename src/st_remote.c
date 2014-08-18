#include <pebble.h>

static Window *window;
static MenuLayer *menu_layer;

static AppSync sync;
static uint8_t sync_buffer[64];

// outgoing commands
typedef enum {
	CMD_KEY_TOGGLE,
	CMD_KEY_REFRESH,
  CMD_KEY_ACTIVATE,
} DeviceCommands;
static GBitmap *switch_icons[2];
static GBitmap *phrase_icon;

typedef enum {
	TYPE_SWITCH = 0x0,
	TYPE_LOCK = 0x1,
  TYPE_PHRASE = 0x2,
} DeviceType;

enum DeviceKey {
	DEVICE_TYPE_KEY = 0x0,		// TUPLE_INT
	DEVICE_ID_KEY = 0x1,			// TUPLE_CSTRING
	DEVICE_LABEL_KEY = 0x2,		// TUPLE_CSTRING
	DEVICE_STATE_KEY = 0x3,		// TUPLE_CSTRING
};

#define MAX_DEVICES (10)
#define MAX_LABEL_LENGTH (16)

typedef struct {
	DeviceType type;
	char id[37];
	char label[MAX_LABEL_LENGTH];
	int state;
} Device;

static Device devices[MAX_DEVICES];
static int device_count = 0;

static Device* get_device_at_index(int index) {
	if (index < 0 || index >= MAX_DEVICES) {
		return NULL;
	}
	
	return &devices[index];
}

static void add_device(DeviceType type, char *id, char *label, int state) {
	if (device_count >= MAX_DEVICES) {
		return;
	}
	
	devices[device_count].type = type;
	strcpy(devices[device_count].id, id);
	strcpy(devices[device_count].label, label);
	devices[device_count].state = state;
	device_count++;
}

static void add_or_update_device(DeviceType type, char *id, char *label, int state) {
	int index = -1;
	for (int i=0; i<device_count; i++) {
		if (strcmp(devices[i].id, id) == 0) {
			index = i;
			devices[index].state = state;
		}
	}

	if (index >= 0) {
		menu_layer_reload_data(menu_layer);
		return;
  } else {
		add_device(type, id, label, state);
	}
}

static void flush_devices() {
	for (int i=0; i<=device_count; i++) {
		strcpy(devices[i].id, "");
	}
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  return 44;
}

static void draw_row_callback(GContext* ctx, Layer *cell_layer, MenuIndex *cell_index, void *data) {
  Device* device;
  const int index = cell_index->row;

  if ((device = get_device_at_index(index)) == NULL) {
    return;
  }
  if (device->type == TYPE_PHRASE) {
    menu_cell_basic_draw(ctx, cell_layer, device->label, NULL, phrase_icon);    
  } else {
    menu_cell_basic_draw(ctx, cell_layer, device->label, NULL, switch_icons[device->state]);
  }
}

static uint16_t get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return device_count;
}

static void select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  const int index = cell_index->row;
  DictionaryIterator *iter;

	APP_LOG(APP_LOG_LEVEL_DEBUG, "select: %d", cell_index->row);
	
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
	// toggle device
  // todo_list_toggle_state(index);
  DeviceCommands cmd;
  if (get_device_at_index(index)->type == TYPE_PHRASE) {
    cmd = CMD_KEY_ACTIVATE;
  } else {
    cmd = CMD_KEY_TOGGLE;
  }
  if (dict_write_cstring(iter, cmd, get_device_at_index(index)->id) != DICT_OK) {
    return;
  }
  app_message_outbox_send();

  //menu_layer_reload_data(menu_layer);
}

static void select_long_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
	return;
	/*
	const int index = cell_index->row;
  DictionaryIterator *iter;

  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  todo_list_delete(index);
  if (dict_write_uint8(iter, TODO_KEY_DELETE, index) != DICT_OK) {
    return;
  }
  app_message_outbox_send();

  menu_layer_reload_data(menu_layer);
*/
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *type_tuple = dict_find(iter, DEVICE_TYPE_KEY);
  Tuple *id_tuple = dict_find(iter, DEVICE_ID_KEY);
  Tuple *label_tuple = dict_find(iter, DEVICE_LABEL_KEY);
  Tuple *state_tuple = dict_find(iter, DEVICE_STATE_KEY);

	APP_LOG(APP_LOG_LEVEL_DEBUG, "rcvd: %s", label_tuple->value->cstring);
  if (type_tuple && id_tuple && label_tuple && state_tuple) {
		add_or_update_device(type_tuple->value->uint8, id_tuple->value->cstring, label_tuple->value->cstring, state_tuple->value->uint8);
  }

  menu_layer_reload_data(menu_layer);
}

char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}
static void in_dropped_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "in msg dropped: %s", translate_error(reason));
}

static void out_dropped_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "out msg dropped: %s", translate_error(reason));
}

static void out_sent_handler(DictionaryIterator *iter, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "out msg sent.");
}

static void app_message_init(void) {
  // Reduce the sniff interval for more responsive messaging at the expense of
  // increased energy consumption by the Bluetooth module
  // The sniff interval will be restored by the system after the app has been
  // unloaded
  app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
  // Register message handlers
  app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_failed(out_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
  // Init buffers
  app_message_open(128, 64);
}

static void window_load(Window *window) {
	switch_icons[0] = gbitmap_create_with_resource(RESOURCE_ID_SWITCH_OFF);
	switch_icons[1] = gbitmap_create_with_resource(RESOURCE_ID_SWITCH_ON);
	phrase_icon = gbitmap_create_with_resource(RESOURCE_ID_PHRASE);

  Layer *window_layer = window_get_root_layer(window);
  GRect window_frame = layer_get_frame(window_layer);

  menu_layer = menu_layer_create(window_frame);
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
    .get_cell_height = (MenuLayerGetCellHeightCallback) get_cell_height_callback,
    .draw_row = (MenuLayerDrawRowCallback) draw_row_callback,
    .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback) get_num_rows_callback,
    .select_click = (MenuLayerSelectCallback) select_callback,
    .select_long_click = (MenuLayerSelectCallback) select_long_callback
  });
  menu_layer_set_click_config_onto_window(menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
//    .unload = window_unload,
  });

	app_message_init();
	
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
  menu_layer_destroy(menu_layer);
	gbitmap_destroy(switch_icons[0]);
	gbitmap_destroy(switch_icons[1]);
	gbitmap_destroy(phrase_icon);
}


int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}

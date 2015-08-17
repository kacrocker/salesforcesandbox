// ******************************************************************************************
// *  Salesforce1 Pebble Dashbaord Sample App
// *  See documentation and disclaim that are on GitHub before use
// ******************************************************************************************
#include "pebble.h"
#define NUM_MENU_SECTIONS 1
#define NUM_MENU_ICONS 3
#define NUM_FIRST_MENU_ITEMS 3
#define NUM_REPORTS 3

static Window *window;
TextLayer *text_layer;
static MenuLayer *menu_layer;

// Create the array for storing the current Report details and counters
static int current_report = 0;          //Identifies which is the current report
static int load_report = NUM_REPORTS;   //Used to loop when all reports need to be loaded
static char *sf_report_name[NUM_REPORTS], *sf_report_value[NUM_REPORTS];

enum {
  KEY_MSG_TYPE = 0,
  KEY_MSG_NAME = 1,
  KEY_MSG_VALUE = 2,
  KEY_DASHBOARD = 3
};

// * ************************************************************************************** refresh_dashborad
void refresh_dashboard(const int iReport){
  //This is the packaging up of the data to send which dashboard the javascript code needs to refresh
  static char sprc[] = "9";
  snprintf(sprc, sizeof(sprc), "%d", iReport);  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "SF1 PB: refresh_dashboard start");
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, 0, 0x1);
	dict_write_cstring(iter, KEY_DASHBOARD, sprc);  
	dict_write_end(iter);
  app_message_outbox_send();
}
// * ************************************************************************************** refresh_dashborad

void process_tuple(Tuple *t)
{
	//Get key for the tuple
	int key = t->key;
  //int cav = 0;  //Contact Array Value

	//Get integer value, if present
	int value = t->value->int32;

	//Get string value, if present
	char string_value[41];
	strcpy(string_value, t->value->cstring);  
	//Decide what to do with the values passed in from the JavaScript
	switch(key) {
		case KEY_MSG_TYPE:
      //The JS app passes back which row this report should be displayed    
      current_report = value;
 			break;
		case KEY_MSG_NAME:
      //Store the Name of the report in the array
      strcpy(sf_report_name[current_report],string_value);
			break;
		case KEY_MSG_VALUE:
      //The report value is passed in as a string; store it in the array
      strcpy(sf_report_value[current_report],string_value);
      break;
    case KEY_DASHBOARD:
      load_report = 0;
      break;
    default: APP_LOG(APP_LOG_LEVEL_DEBUG, "SF1 PB: Unknown key for Tuple");
  }
        layer_mark_dirty(menu_layer_get_layer(menu_layer)); 
}
//This code is called whenever a new inbound message is recieved from the phone
static void in_received_handler(DictionaryIterator *iter, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "SF1 PB: in_received_handler");
	(void) context;   
  Tuple *t = dict_read_first(iter);
	if(t)
	{
    process_tuple(t);
	}
	while(t != NULL)
	{
		t = dict_read_next(iter);
		if(t)
		{
			process_tuple(t);
		}
	}

  if (load_report >= NUM_REPORTS) {
    //All reports are current so pulse once to indicate the load is complete
      vibes_short_pulse();
      layer_mark_dirty(menu_layer_get_layer(menu_layer)); 
  } else {
    //This is used to manage loading multiple rows if the user is already logged on
      APP_LOG(APP_LOG_LEVEL_DEBUG, "SF1 PB: Load Another Row");
      refresh_dashboard(load_report);    
      load_report = load_report + 1;
      layer_mark_dirty(menu_layer_get_layer(menu_layer)); 
  }
}

// A callback is used to specify the amount of sections of menu items
// With this, you can dynamically add and remove sections
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

// Each section has a number of items;  we use a callback to specify this
// You can also dynamically add and remove items using this
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      return NUM_FIRST_MENU_ITEMS;
    default:
      return 0;
  }
}

// A callback is used to specify the height of the section header
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  // This is a define provided in pebble.h that you may use for the default height
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

// Here we draw the Header of the screen on the Pebble watch
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
      menu_cell_basic_header_draw(ctx, cell_layer, "Salesforce1 Platform");
}

// This is the menu item draw callback where you specify what each item should look like on the screen
// Since all our lines are coming from an array, we don't need a case statement like most sample codes use.
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  menu_cell_basic_draw(ctx, cell_layer, sf_report_value[cell_index->row], sf_report_name[cell_index->row], NULL);   
}
// * ************************************************************************************** send_int
void send_int(uint8_t key, uint8_t cmd)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "SF1 PB: send_int start");
	DictionaryIterator *iter;
 	app_message_outbox_begin(&iter);
 	Tuplet value = TupletInteger(key, cmd);
 	dict_write_tuplet(iter, &value);
 	app_message_outbox_send();
}
// * ************************************************************************************** send_int
// * ************************************************************************************** menu_select_callback
// Here we capture when a user selects a menu item
void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Use the row to specify which item will receive the select action
  // Send a quick pulse to acknowledge the button click
  vibes_short_pulse();
  refresh_dashboard(cell_index->row);
}
// * ************************************************************************************** menu_select_callback
// * ************************************************************************************** window_load
// This initializes the menu upon window load
void window_load(Window *window) {
  	    APP_LOG(APP_LOG_LEVEL_DEBUG, "PEBBLE : window_load");
  
  // Now we prepare to initialize the menu layer
  // We need the bounds to specify the menu layer's viewport size
  // In this case, it'll be the same as the window's
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  menu_layer = menu_layer_create(bounds);

  // Set all the callbacks for the menu layer
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(menu_layer, window);

  // Add it to the window for display
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}
// * ************************************************************************************** window_load
// * ************************************************************************************** window_unload
void window_unload(Window *window) {
  // Destroy the menu layer
  menu_layer_destroy(menu_layer);
}
// * ************************************************************************************** window_unload
// * ************************************************************************************** MAIN
// * ************************************************************************************** MAIN


int main(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "PEBBLE : >>main<<");
  window = window_create();
  // Setup the window handlers
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
 
//Default information to display on the screen before you link to the phone
  sf_report_value[0] = "Welcome to                    ";
  sf_report_name[0] =  "a sample app for              ";
  sf_report_value[1] = "Salesforce1                   ";
  sf_report_name[1] =  "displaying info               ";
  sf_report_value[2] = "on your Pebble                ";
  sf_report_name[2] =  "wrist watch                   ";
    
  //Register AppMessage events
	app_message_register_inbox_received(in_received_handler);					 
	app_message_open(512, 512);		//Large input and output buffer sizes
  
  //This will cause the javascript to get login to Salesforce and get a session token
  APP_LOG(APP_LOG_LEVEL_DEBUG, "PEBBLE : send_int");
	send_int(9, 9);

  window_stack_push(window, true);
  app_event_loop();
  window_destroy(window);
}

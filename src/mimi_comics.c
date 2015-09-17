#include <pebble.h>

Window* my_window = NULL;

//Image Display
BitmapLayer* bitmap_layer = NULL;
GBitmap* gbitmap_ptr = NULL;

//Transition animations
static Layer* animation_layer = NULL;
static PropertyAnimation* prop_animation_slide_left = NULL;
static PropertyAnimation* prop_animation_slide_up = NULL;
static uint8_t* animation_style_config = NULL;

#define MAX(A,B) ((A>B) ? A : B)
#define MIN(A,B) ((A<B) ? A : B)

#define RESUME_KEY 0
#define LEFT '1'

//Image indexes and count
#define RESOURCE_ID_IMAGE_OFFSET 2
int image_index = RESOURCE_ID_IMAGE_OFFSET;
int max_images = 0; //automatically detected

static void do_animation(void){
  if (animation_style_config[image_index - RESOURCE_ID_IMAGE_OFFSET] == LEFT) {
    Animation *animation_slide_left = 
      animation_clone((Animation*)prop_animation_slide_left);
    animation_schedule(animation_slide_left);
  } else {
    Animation *animation_slide_up = 
      animation_clone((Animation*)prop_animation_slide_up);
    animation_schedule(animation_slide_up);
  }
}

static void load_image_resource(uint32_t resource_id){
  if (gbitmap_ptr) {
    gbitmap_destroy(gbitmap_ptr);
    gbitmap_ptr = NULL;
  }
  gbitmap_ptr = gbitmap_create_with_resource(resource_id);
  bitmap_layer_set_bitmap(bitmap_layer, gbitmap_ptr);
}

static void increment_image(void) {
  image_index = ((image_index + 1 - RESOURCE_ID_IMAGE_OFFSET) >= max_images) ? 
    (RESOURCE_ID_IMAGE_OFFSET) : (image_index + 1);
  load_image_resource(image_index);
  do_animation();
}

static void decrement_image(void) {
  image_index = ((image_index - 1) < RESOURCE_ID_IMAGE_OFFSET) ? 
    (RESOURCE_ID_IMAGE_OFFSET + max_images - 1) : (image_index - 1);
  load_image_resource(image_index);
  layer_mark_dirty(animation_layer);
}

static void init_animation(GRect bounds) {
  GRect right_image_bounds = (GRect){
    .origin = (GPoint){.x=bounds.size.w,.y=0},
    .size = bounds.size};

  GRect bottom_image_bounds = (GRect){
    .origin = (GPoint){.x=0,.y=bounds.size.h},
    .size = bounds.size};

  // Setup slide left animation
  GRect left_stop = GRect(0,0,bounds.size.w, bounds.size.h);
  prop_animation_slide_left = property_animation_create_layer_frame( 
    animation_layer, &right_image_bounds, &left_stop);

  animation_set_duration((Animation*)prop_animation_slide_left, 1500);
  animation_set_curve((Animation*)prop_animation_slide_left, 
    AnimationCurveEaseInOut);

  // Setup slide up animation
  GRect up_stop = GRect(0,0,bounds.size.w, bounds.size.h);
  prop_animation_slide_up = property_animation_create_layer_frame( 
    animation_layer, &bottom_image_bounds, &up_stop);

  animation_set_duration((Animation*)prop_animation_slide_up, 1500);
  animation_set_curve((Animation*)prop_animation_slide_up, 
    AnimationCurveEaseInOut);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  decrement_image();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  increment_image();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void animation_hack_callback(void *data) {
  load_image_resource(image_index);
  layer_mark_dirty(animation_layer);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  //Add layers from back to front (background first)

  //Initialise animations
  animation_layer = layer_create(bounds);
  layer_add_child(window_layer, animation_layer);
  init_animation(bounds);
  
  //Create bitmap layer for background image
  bitmap_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_compositing_mode(bitmap_layer, GCompOpSet);
  //Load initial bitmap image
  load_image_resource(image_index);

  //Add bitmap_layer to animation layer
  layer_add_child(animation_layer, bitmap_layer_get_layer(bitmap_layer));
    
  app_timer_register(10, animation_hack_callback, NULL);  // hack
}

static void window_unload(Window *window) {
}


void handle_init(void) {
  //Load the resource animation configuration file
  ResHandle animation_style_config_handle = 
    resource_get_handle(RESOURCE_ID_ANIMATION_CONFIG);
  int animation_style_config_bytes = resource_size(animation_style_config_handle);
  animation_style_config = (uint8_t*)malloc(animation_style_config_bytes);
  
  resource_load(animation_style_config_handle, 
                animation_style_config, animation_style_config_bytes);

  //Discover how many images from base index
  while (resource_get_handle(RESOURCE_ID_IMAGE_OFFSET + max_images)) {
    max_images++;
  }
  printf("max images:%d",max_images);

  //Check if we can resume where we left off
  if(persist_exists(RESUME_KEY)){
    image_index = persist_read_int(RESUME_KEY);
    if (image_index - RESOURCE_ID_IMAGE_OFFSET >= max_images) {
      image_index = RESOURCE_ID_IMAGE_OFFSET;
    }
  }

  light_enable(true);
  my_window = window_create();
  //Allows compositing new image animation over last image displayed
  window_set_background_color(my_window, GColorClear);  
  window_set_click_config_provider(my_window, click_config_provider);
  window_set_window_handlers(my_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  window_stack_push(my_window, false/*animated*/);
}

void handle_deinit(void) {
    persist_write_int(RESUME_KEY,image_index);
    bitmap_layer_destroy(bitmap_layer);
	  window_destroy(my_window);
}


int main(void) {
	  handle_init();
	  app_event_loop();
	  handle_deinit();
}

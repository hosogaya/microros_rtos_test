#include <arduino_freertos.h>
#if _GCC_VERSION < 60100
#error "Compiler too old for std::thread support with FreeRTOS."
#endif

#include <thread>
#include <chrono>

#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>

#if !defined(MICRO_ROS_TRANSPORT_ARDUINO_SERIAL)
#error This example is only avaliable for Arduino framework with serial transport.
#endif

rcl_publisher_t publisher;
rcl_subscription_t subscriber;
std_msgs__msg__Int32 pub_msg;
std_msgs__msg__Int32 sub_msg;
std_msgs__msg__Int32 msg_buffer;

rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void callback(void* msg)
{
  std_msgs__msg__Int32* m = (std_msgs__msg__Int32*)msg;
  if (m == nullptr) return;
  sub_msg = *m;
}

static void microros_task(void*)
{ 
  pub_msg = sub_msg;
  rcl_publish(&publisher, &pub_msg, NULL);
  vTaskDelay(pdMS_TO_TICKS(100));
}

static void task(void*)
{
  vTaskDelay(pdMS_TO_TICKS(100));
}


FLASHMEM __attribute__((noinline)) void setup() {
  ::Serial.begin(115200);
  delay(2'000);

  if (CrashReport) {
    ::Serial.print(CrashReport);
    ::Serial.println();
    ::Serial.flush();
  }

  ::Serial.println(PSTR("\r\nBooting FreeRTOS kernel " tskKERNEL_VERSION_NUMBER ". Built by gcc " __VERSION__ " (newlib " _NEWLIB_VERSION ") on " __DATE__ ". ***\r\n"));

  set_microros_serial_transports(::Serial);
  allocator = rcl_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "micro_ros_platformio_node", "", &support);
  rclc_publisher_init_default(
    &publisher, 
    &node, 
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "micro_ros_platformio_node_publisher"
  );
  rclc_subscription_init_default(
    &subscriber, 
    &node, 
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "micro_ros_platformio_node_subscriber"
  );

  // executor = rclc_executor_get_zero_initialized_executor();
  // rclc_executor_init(&executor, &support.context, 1, &allocator);
  // rclc_executor_add_subscription(&executor, &subscriber, &msg_buffer, &callback, ON_NEW_DATA);

  xTaskCreate(task, "task", 128, nullptr, 2, nullptr);
  xTaskCreate(microros_task, "microros task", 128, nullptr, 2, nullptr);

  ::Serial.println("setup(): starting scheduler...");
  ::Serial.flush();

  vTaskStartScheduler();
}

void loop() {}
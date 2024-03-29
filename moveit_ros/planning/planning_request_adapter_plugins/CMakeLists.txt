set(SOURCE_FILES
  src/empty.cpp
  src/fix_start_state_bounds.cpp
  src/fix_start_state_collision.cpp
  src/fix_start_state_path_constraints.cpp
  src/fix_workspace_bounds.cpp
  src/add_ruckig_traj_smoothing.cpp
  src/add_time_optimal_parameterization.cpp
  src/resolve_constraint_frames.cpp
)

add_library(moveit_default_planning_request_adapter_plugins SHARED ${SOURCE_FILES})

set_target_properties(moveit_default_planning_request_adapter_plugins PROPERTIES VERSION "${${PROJECT_NAME}_VERSION}")
ament_target_dependencies(moveit_default_planning_request_adapter_plugins
  Boost
  moveit_core
  rclcpp
  pluginlib
)

add_executable(moveit_list_request_adapter_plugins src/list.cpp)
ament_target_dependencies(moveit_list_request_adapter_plugins
  Boost
  moveit_core
  rclcpp
  pluginlib
)

install(TARGETS moveit_list_request_adapter_plugins
  DESTINATION lib/${PROJECT_NAME}
)

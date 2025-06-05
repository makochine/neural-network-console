cmake_minimum_required(VERSION 3.0)

add_executable(sdeep_console_cli_util
  src/neural_network_console/ioutils/caffe_utility.cpp
  src/neural_network_console/ioutils/nn_document_yamlutility.cpp
  src/neural_network_console/ioutils/nnabla_utility.cpp
  src/neural_network_console/ioutils/nn_document_yamlutility.cpp
  src/neural_network_console/ioutils/nncplugin.cpp

  src/neural_network_console/utils/ffastini.cpp
  src/neural_network_console/utils/file_util.cpp
  src/neural_network_console/utils/fstring.cpp
  src/neural_network_console/utils/gaussian_process.cpp

  src/neural_network_console/csv_dataset.cpp
  src/neural_network_console/nn_config.cpp
  src/neural_network_console/nn_dataset.cpp
  src/neural_network_console/nn_document.cpp
  src/neural_network_console/nn_document_converter_setting.cpp
  src/neural_network_console/nn_layer_component.cpp
  src/neural_network_console/nn_script.cpp
  src/neural_network_console/nn_structure.cpp
  src/neural_network_console/nn_structure_search_policy.cpp
  src/neural_network_console/nnstructure_to_vector.cpp
  src/neural_network_console/sdeep_console_global_settings.cpp
  src/neural_network_console/solver_component.cpp

  src/sdeep_console_cli_util/arrange_layers.cpp
  src/sdeep_console_cli_util/complete.cpp
  src/sdeep_console_cli_util/create_prototxt.cpp
  src/sdeep_console_cli_util/create_result_ini.cpp
  src/sdeep_console_cli_util/error_check.cpp
  src/sdeep_console_cli_util/export_caffe.cpp
  src/sdeep_console_cli_util/export_python_code.cpp
  src/sdeep_console_cli_util/get_parameter_name.cpp
  src/sdeep_console_cli_util/import_caffe.cpp
  src/sdeep_console_cli_util/import_nntxt.cpp
  src/sdeep_console_cli_util/mutate.cpp
  src/sdeep_console_cli_util/structure_search.cpp

  src/sdeep_console_cli_util/main.cpp)

include_directories(src/neural_network_console)
include_directories(src/neural_network_console/ioutils)
include_directories(src/neural_network_console/utils)

set(CMAKE_CXX_FLAGS "-Wno-error=narrowing")
set_property(TARGET sdeep_console_cli_util PROPERTY CXX_STANDARD 11)

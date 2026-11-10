add_executable(p3dcheck P3dCheck.cc)
target_link_libraries(p3dcheck PRIVATE tk_p3d)

add_executable(rcftool RcfTool.cc)
target_link_libraries(rcftool PRIVATE tk_rcf)

add_executable(fsccheck FscCheck.cc)
target_link_libraries(fsccheck PRIVATE tk_nis)

add_executable(rigcheck RigCheck.cc)
target_link_libraries(rigcheck PRIVATE tk_rig)

add_executable(weightcheck WeightCheck.cc)
target_link_libraries(weightcheck PRIVATE tk_rig)

add_executable(charpack CharPack.cc)
target_link_libraries(charpack PRIVATE tk_rig)

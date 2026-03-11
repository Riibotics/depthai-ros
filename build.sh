#!/bin/bash
Help()
{
    echo "Build minimal depthai-ros workspace"
    echo
    echo "Build options:"
    echo "-s [1]   Set to 1 to build sequentially (longer, but saves RAM & CPU)"
    echo "-r [0]   Set to 1 to build in Debug mode. (RelWithDebInfo by default)"
    echo "-m [0]   Set to 1 to build with --merge-install option."
    echo
}

sequential=1
release=0
merge=0
build_type=Release
install_type=symlink-install
while getopts ":h:s:r:m:" option; do
   case $option in
      h)
         Help
         exit;;
      s)
         sequential=$OPTARG;;
      r)
         release=$OPTARG;;
      m)
         merge=$OPTARG;;
     \?)
         echo "Error: Invalid option"
         exit;;
   esac
done

if [ "$release" == 0 ]
then
    build_type="RelWithDebInfo"
fi

if [ "$merge" == 1 ]
then
    install_type="merge-install"
fi

echo "Build type: $build_type, Install type: $install_type"
PKGS="depthai_bridge depthai_ros_driver depthai-ros"

if [ "$sequential" == 1 ]
then
    echo "Sequential build" && \
    MAKEFLAGS="-j1 -l1" colcon build \
        --$install_type \
        --executor sequential \
        --packages-select $PKGS \
        --cmake-args -DCMAKE_BUILD_TYPE=$build_type \
         -DBUILD_TESTING=OFF \
         -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
         -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
         -DBUILD_SHARED_LIBS=ON
else
    echo "Parallel build" && \
    colcon build \
    --$install_type \
    --packages-select $PKGS \
    --cmake-args -DCMAKE_BUILD_TYPE=$build_type \
    -DBUILD_TESTING=OFF \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_SHARED_LIBS=ON
fi

#!/bin/bash

DIR_WITH_3D_PARTY_PLUGINS="./"
if [ $# -ne 1 ]
then 
  echo "Please provide a path argument to the path that contains the 3dparty directory ( might be ./ )"
else
 DIR_WITH_3D_PARTY_PLUGINS=$1 
fi


if [ -d "$DIR_WITH_3D_PARTY_PLUGINS/libfreenect" ]
then
echo "Linking to freenect libs"  
ln -s "$DIR_WITH_3D_PARTY_PLUGINS/libfreenect/build/lib/libfreenect.so"
ln -s "$DIR_WITH_3D_PARTY_PLUGINS/libfreenect/build/lib/libfreenect_sync.so"  
ln -s "$DIR_WITH_3D_PARTY_PLUGINS/libfreenect/build/lib/libfreenect.so.0.1"
ln -s "$DIR_WITH_3D_PARTY_PLUGINS/libfreenect/build/lib/libfreenect_sync.so.0.1"  
else
 echo "Could not find libfreenect directory"
fi

if [ -d "$DIR_WITH_3D_PARTY_PLUGINS/OpenNI" ]
then
echo "Linking to OpenNI1 libs"
ln -s "$DIR_WITH_3D_PARTY_PLUGINS/OpenNI/Platform/Linux/Bin/x64-Release/libOpenNI.so"
ln -s "$DIR_WITH_3D_PARTY_PLUGINS/OpenNI/Platform/Linux/Bin/x64-Release/libOpenNI.jni.so" 
ln -s "$DIR_WITH_3D_PARTY_PLUGINS/OpenNI/Platform/Linux/Bin/x64-Release/libnimCodecs.so"
ln -s "$DIR_WITH_3D_PARTY_PLUGINS/OpenNI/Platform/Linux/Bin/x64-Release/libnimMockNodes.so"
ln -s "$DIR_WITH_3D_PARTY_PLUGINS/OpenNI/Platform/Linux/Bin/x64-Release/libnimRecorder.so"
else
 echo "Could not find OpenNI1 directory"
fi


if [ -d "$DIR_WITH_3D_PARTY_PLUGINS/OpenNI2" ]
then
echo "Linking to OpenNI2 libs"
ln -s "$DIR_WITH_3D_PARTY_PLUGINS/OpenNI2/Bin/x64-Release/OpenNI2/"
ln -s "$DIR_WITH_3D_PARTY_PLUGINS/OpenNI2/Config/OpenNI.ini"
ln -s "$DIR_WITH_3D_PARTY_PLUGINS/OpenNI2/Config/PS1080.ini"
ln -s "$DIR_WITH_3D_PARTY_PLUGINS/OpenNI2/Bin/x64-Release/libOpenNI2.so"
else
 echo "Could not find OpenNI2 directory"
fi

exit 0

#!/bin/bash

BINARIES_THAT_NEED_LIBS="grabber viewer grabber_mux grabber_segment acquisitionBroadcast redist"

if [ -d libfreenect ]
then
echo "Freenect appears to already exist .."
else
  echo "Do you want to download freenect ? " 
  echo
  echo -n " (Y/N)?"
  read answer
  if test "$answer" != "N" -a "$answer" != "n";
  then
     sudo pacman -S libxmu libxi freeglut libusb
     git clone git://github.com/OpenKinect/libfreenect.git
     cd libfreenect
     mkdir build
     cd build
     cmake ..
     make
     cd ../../ 


     #should be at 3dparty dir
     cd ..  
     #should be at root dir
     for f in $BINARIES_THAT_NEED_LIBS
           do  
             if [ -d $f ]
              then
               cd $f 
               ln -s ../3dparty/libfreenect/build/lib/libfreenect.so
               ln -s ../3dparty/libfreenect/build/lib/libfreenect_sync.so 
               cd ..
             else
              echo "Could not create links for $f"
             fi
           done

     #should be at 3dparty dir
     cd 3dparty   
  
  fi
fi


#wget http://www.openni.org/wp-content/uploads/2012/12/OpenNI-Bin-Dev-Linux-Arm-v1.5.4.0.tar.zip
#unzip OpenNI-Bin-Dev-Linux-Arm-v1.5.4.0.tar.zip
#rm OpenNI-Bin-Dev-Linux-Arm-v1.5.4.0.tar.zip
#tar xvjf OpenNI-Bin-Dev-Linux-Arm-v1.5.4.0.tar.bz2 
#--
#ln -s OpenNI-Bin-Dev-Linux-Arm-v1.5.4.0/ OpenNI
#mkdir -p Platform/Linux/Bin/
#cd Platform/Linux/Bin/
#ln -s ../../../Lib/ x64-Release
#cd ../../../../


if [ -d OpenNI ]
then
echo "OpenNI1 appears to already exist .."
else
  echo "Do you want to download OpenNI1 ? " 
  echo
  echo -n " (Y/N)?"
  read answer
  if test "$answer" != "N" -a "$answer" != "n";
  then
     sudo pacman -S libxmu libxi freeglut libusb
     #git clone git://github.com/OpenNI/OpenNI.git 
     #cd OpenNI/Platform/Linux/CreateRedist
     #./RedistMaker
     #cd ../Redist
     ##sudo ./install.sh 
     #cd ../../../../

     yaourt -G openni-git
     cd openni-git
     sed -e 's/_gitbranch="unstable"/_gitbranch="master"/g' PKGBUILD > PKGBUILD.tmp && mv PKGBUILD.tmp PKGBUILD
     makepkg -i
     cd ..
     mv openni-git/src/openni-build OpenNI
     rm -rf openni-git


     #should be at 3dparty dir
     cd ..  
     #should be at root dir
     for f in $BINARIES_THAT_NEED_LIBS
           do  
             if [ -d $f ]
              then
               cd $f 
               ln -s ../3dparty/OpenNI/Platform/Linux/Bin/x64-Release/libOpenNI.so
               ln -s ../3dparty/OpenNI/Platform/Linux/Bin/x64-Release/libOpenNI.jni.so  
               ln -s ../3dparty/OpenNI/Platform/Linux/Bin/x64-Release/libnimCodecs.so 
               ln -s ../3dparty/OpenNI/Platform/Linux/Bin/x64-Release/libnimMockNodes.so 
               ln -s ../3dparty/OpenNI/Platform/Linux/Bin/x64-Release/libnimRecorder.so 
               cd ..
             else
              echo "Could not create links for $f"
             fi
           done

     #should be at 3dparty dir
     cd 3dparty   

  fi
fi

#wget http://www.openni.org/wp-content/uploads/2013/01/OpenNI-Linux-Arm-2.1.0.tar.zip
#unzip OpenNI-Linux-Arm-2.1.0.tar.zip 
#rm OpenNI-Linux-Arm-2.1.0.tar.zip 
#tar xvjf OpenNI-Linux-Arm-2.1.0.tar.bz2 
#--
#ln -s OpenNI-2.1.0-arm/ OpenNI2
#mkdir Bin 
#cd Bin
#ln -s ../Redist/ x64-Release
#cd ..

if [ -d OpenNI2 ]
then
echo "OpenNI2 appears to already exist .."
else
  echo "Do you want to download OpenNI2 ? " 
  echo
  echo -n " (Y/N)?"
  read answer
  if test "$answer" != "N" -a "$answer" != "n";
  then
     sudo pacman -S libxmu libxi freeglut libusb
     #git clone git://github.com/OpenNI/OpenNI2.git
     #cd OpenNI2
     #make 
     #cd ..
     
     yaourt -G openni2
     cd openni2
     makepkg -i
     cd ..
     mv openni2/src/OpenNI2 .
     rm -rf openni2

     #should be at 3dparty dir
     cd ..  
     #should be at root dir
     for f in $BINARIES_THAT_NEED_LIBS
           do  
             if [ -d $f ]
              then
               cd $f 
               ln -s ../3dparty/OpenNI2/Bin/x64-Release/OpenNI2/  
               ln -s ../3dparty/OpenNI2/Config/OpenNI.ini   
               ln -s ../3dparty/OpenNI2/Config/PS1080.ini 
               ln -s ../3dparty/OpenNI2/Bin/x64-Release/libOpenNI2.so 
               cd ..
             else
              echo "Could not create links for $f"
             fi
           done

     #should be at 3dparty dir
     cd 3dparty   

  fi
fi

exit 0

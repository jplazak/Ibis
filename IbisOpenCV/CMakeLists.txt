SET( IBIS_VTKOPENCV_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
SET( IBIS_VTKOPENCV_INCLUDE_DIR ${IBIS_VTKOPENCV_INCLUDE_DIR} PARENT_SCOPE  )

SET( VTKOPENCV_SRC
	vtkopencvbridge.cpp
        ibisopencvmath.cpp
)

SET( VTKOPENCV_HDR
	vtkopencvbridge.h
        ibisopencvmath.h
)

#================================
# Includes dir for vtkIbis libs
#================================
include_directories( ${IBIS_VTK_INCLUDE_DIR} )

#================================
# Define output
#================================
IF (BUILD_SHARED_LIBS)
ADD_LIBRARY( ibisVTKOpenCVBridge SHARED ${VTKOPENCVS_SRC} ${VTKOPENCV_HDR} )
ELSE( BUILD_SHARED_LIBS )
ADD_LIBRARY( ibisVTKOpenCVBridge ${VTKOPENCV_SRC} ${VTKOPENCV_HDR} )
ENDIF( BUILD_SHARED_LIBS )

# install
if( IBIS_INSTALL_LIBS_AND_HEADERS )
    INSTALL_TARGETS(/lib ibisVTKOpenCVBridge)
    INSTALL_FILES(/include FILES ${VTKOPENCV_HDR} )
endif( IBIS_INSTALL_LIBS_AND_HEADERS )


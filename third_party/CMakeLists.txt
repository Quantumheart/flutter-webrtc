include(ExternalProject)

set(LIBWEBRTC_DIR "${CMAKE_CURRENT_LIST_DIR}/libwebrtc")
set(ZIPFILE "${CMAKE_CURRENT_LIST_DIR}/downloads/libwebrtc.zip")
set(DOWNLOAD_URL "https://github.com/Quantumheart/flutter-webrtc/releases/download/v1.3.0-lattice/libwebrtc.zip")

if(EXISTS "${LIBWEBRTC_DIR}/lib" AND EXISTS "${LIBWEBRTC_DIR}/include")
  message(TRACE "libwebrtc already present, skipping download.")
else()
  if(NOT EXISTS "${ZIPFILE}")
    message(NOTICE "download: ${DOWNLOAD_URL}")
    file(DOWNLOAD "${DOWNLOAD_URL}"
        ${ZIPFILE}
        STATUS download_status
        LOG download_log)

    if(NOT download_status EQUAL 0)
      message(FATAL_ERROR "Failed to download dependency: ${download_log}")
    endif()
  endif()

  file(ARCHIVE_EXTRACT INPUT ${ZIPFILE} DESTINATION "${CMAKE_CURRENT_LIST_DIR}")
endif()


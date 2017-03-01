# define sources
set( PluginSrc osccaptureplugininterface.cpp osccapturewidget.cpp ip/posix/NetworkingUtils.cpp
    ip/posix/UdpSocket.cpp ip/IpEndpointName.cpp osc/OscOutboundPacketStream.cpp 
    osc/OscPrintReceivedElements.cpp osc/OscReceivedElements.cpp osc/OscTypes.cpp )

set( PluginHdrMoc osccapturewidget.h osccaptureplugininterface.h ip/IpEndpointName.h
    ip/NetworkingUtils.h ip/PacketListener.h ip/TimerListener.h ip/UdpSocket.h
    osc/MessageMappingOscPacketListener.h osc/OscException.h osc/OscHostEndianness.h
    osc/OscOutboundPacketStream.h osc/OscPacketListener.h osc/OscPrintReceivedElements.h
    osc/OscReceivedElements.h osc/OscTypes.h )

set( PluginUi osccapturewidget.ui )

# Create plugin
DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )

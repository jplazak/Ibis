/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#include "osccaptureplugininterface.h"
#include "osccapturewidget.h"
#include "scenemanager.h"
#include "pointerobject.h"
#include "application.h"
<<<<<<< HEAD
=======
#include "pointsobject.h"
>>>>>>> ddbfe6cd5ae60614e9379acc8fcb1c40774417ca
#include "osc/OscOutboundPacketStream.h"    //Import OSCpack Library
#include "ip/UdpSocket.h"                   //Import OSCpack Library

#define ADDRESS "127.0.0.1"                 //OSC Data Address
#define PORT 8005                           //OSC Data Port
#define OUTPUT_BUFFER_SIZE 1024             //OSC Data Size
<<<<<<< HEAD
=======
UdpTransmitSocket transmitSocket( IpEndpointName( ADDRESS, PORT ) );
>>>>>>> ddbfe6cd5ae60614e9379acc8fcb1c40774417ca

OSCCapturePluginInterface::OSCCapturePluginInterface()
{
    m_tipPosition[0] = 0.0;
    m_tipPosition[1] = 0.0;
    m_tipPosition[2] = 0.0;
<<<<<<< HEAD
=======
    m_pointsId = SceneManager::InvalidId;
>>>>>>> ddbfe6cd5ae60614e9379acc8fcb1c40774417ca
}

OSCCapturePluginInterface::~OSCCapturePluginInterface()
{
}

bool OSCCapturePluginInterface::CanRun()
{
    return true;
}

QWidget * OSCCapturePluginInterface::CreateTab()
{
    OSCCaptureWidget * widget = new OSCCaptureWidget;
    widget->SetPluginInterface( this );

    connect( GetApplication(), SIGNAL(IbisClockTick()), this, SLOT(OnUpdate()) );

<<<<<<< HEAD
=======
    // Create a points object if it doesn't exist already
    if( m_pointsId == SceneManager::InvalidId )
    {
        PointsObject * p = PointsObject::New();
        p->SetName("OSC plugin points");
        p->SetCanChangeParent( false );
        p->SetCanAppendChildren( false );
        p->SetCanEditTransformManually( false );
        p->SetObjectManagedBySystem( true );
        GetSceneManager()->AddObject( p );
        m_pointsId = p->GetObjectID();
    }

    PointsObject * p = PointsObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_pointsId ) );
    Q_ASSERT( p );
    connect( p, SIGNAL(Modified()), this, SLOT(OnPointsModified()) );

>>>>>>> ddbfe6cd5ae60614e9379acc8fcb1c40774417ca
    return widget;
}

bool OSCCapturePluginInterface::WidgetAboutToClose()
{
    disconnect( GetApplication(), SIGNAL(IbisClockTick()), this, SLOT(OnUpdate()) );
    return true;
}

void OSCCapturePluginInterface::OnUpdate()
{
    PointerObject * p = GetSceneManager()->GetNavigationPointerObject();
    if( p )
    {
        double * pos = p->GetTipPosition();
        m_tipPosition[0] = pos[0];
        m_tipPosition[1] = pos[1];
        m_tipPosition[2] = pos[2];
<<<<<<< HEAD
        
    // DEV CHANGES BEGIN
        UdpTransmitSocket transmitSocket( IpEndpointName( ADDRESS, PORT ) );
=======

        //enum TrackerToolState{ Ok, Missing, OutOfVolume, OutOfView, Undefined };
        int p_state = (int)p->GetState();
        
>>>>>>> ddbfe6cd5ae60614e9379acc8fcb1c40774417ca
        char buffer[OUTPUT_BUFFER_SIZE];
        osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
        
        p << osc::BeginBundleImmediate
<<<<<<< HEAD
            << osc::BeginMessage( "/pointerX" ) << m_tipPosition[0] << osc::EndMessage
            << osc::BeginMessage( "/pointerY" ) << m_tipPosition[1] << osc::EndMessage
            << osc::BeginMessage( "/pointerZ" ) << m_tipPosition[2] << osc::EndMessage
        << osc::EndBundle;
        
        transmitSocket.Send( p.Data(), p.Size() );
    //END DEV CHANGES
=======
            << osc::BeginMessage( "/pointerX" ) << ((float)m_tipPosition[0]) << osc::EndMessage
            << osc::BeginMessage( "/pointerY" ) << ((float)m_tipPosition[1]) << osc::EndMessage
            << osc::BeginMessage( "/pointerZ" ) << ((float)m_tipPosition[2]) << osc::EndMessage
             << osc::BeginMessage( "/pointerState" ) << p_state << osc::EndMessage
        << osc::EndBundle;
        
        transmitSocket.Send( p.Data(), p.Size() );
>>>>>>> ddbfe6cd5ae60614e9379acc8fcb1c40774417ca
        
        emit Modified();
    }
}
<<<<<<< HEAD
=======

void OSCCapturePluginInterface::OnPointsModified()
{
    PointsObject * p = PointsObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_pointsId ) );
    Q_ASSERT( p );
    for( int i = 0; i < p->GetNumberOfPoints(); ++i )
    {
        double * pos = p->GetPointCoordinates( i );
        //std::cout << "p " << i << " ( " << pos[0] << ", " << pos[1] << ", " << pos[2] << " )" << std::endl;

        char buffer[OUTPUT_BUFFER_SIZE];
        osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

        p << osc::BeginBundleImmediate
            << osc::BeginMessage( "/pointMarkerX" ) << ((float)pos[0]) << osc::EndMessage
            << osc::BeginMessage( "/pointMarkerY" ) << ((float)pos[1]) << osc::EndMessage
            << osc::BeginMessage( "/pointMarkerZ" ) << ((float)pos[2]) << osc::EndMessage
        << osc::EndBundle;

        transmitSocket.Send( p.Data(), p.Size() );
    }
    std::cout << std::endl;
}
>>>>>>> ddbfe6cd5ae60614e9379acc8fcb1c40774417ca

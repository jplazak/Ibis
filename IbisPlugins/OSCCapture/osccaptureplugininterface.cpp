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
#include "pointsobject.h"
#include "osc/OscOutboundPacketStream.h"    //Import OSCpack Library
#include "ip/UdpSocket.h"                   //Import OSCpack Library
#include <QKeyEvent>

#define ADDRESS "127.0.0.1"                 //OSC Data Address
#define PORT 8005                           //OSC Data Port
#define OUTPUT_BUFFER_SIZE 1024             //OSC Data Size
UdpTransmitSocket transmitSocket( IpEndpointName( ADDRESS, PORT ) );
int counter = 0;

double trialPoints[16][3] = {
{ 0, 5, 50 },
{ 50, 5, 50 },
{ 0, 45, 50 },
{ 50, 45, 50 },
{ 0, 50, 45 },
{ 50, 50, 45 },
{ 0, 50, 5 },
{ 50, 50, 5 },
{ 0, 45, 0 },
{ 50, 45, 0 },
{  0, 5, 0 },
{  50, 5, 0 },
{  0, 0, 45 },
{  50, 0, 45 },
{  0, 0, 5 },
{  50, 0, 5 }
};

OSCCapturePluginInterface::OSCCapturePluginInterface()
{
    m_tipPosition[0] = 0.0;
    m_tipPosition[1] = 0.0;
    m_tipPosition[2] = 0.0;
    m_pointsId = SceneManager::InvalidId;
//    double m_pointCoord[3] = {0.0,0.0,0.0};
//    m_pointCoord[0] = 0.0;
//    m_pointCoord[1] = 0.0;
//    m_pointCoord[2] = 0.0;
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

    //Add Listener for Keyboard inputs
    GetApplication()->AddGlobalEventHandler( this );

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
    double testArray[3] = {0.0,0.0,0.0};
    p->AddPoint("",testArray);              //Points require a name (empty string here) and an array of values
    p->SetPointCoordinates(0, testArray);

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
        //Get Pointer Position
        double * pos = p->GetTipPosition();
        m_tipPosition[0] = pos[0];
        m_tipPosition[1] = pos[1];
        m_tipPosition[2] = pos[2];

        //Get Pointer State
        //enum TrackerToolState{ Ok, Missing, OutOfVolume, OutOfView, Undefined };
        int p_state = (int)p->GetState();

        //Get Marker Coords
        double m_pointCoord[3] = {0.0,0.0,0.0};
        PointsObject * m = PointsObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_pointsId ) );
        Q_ASSERT( m );
        m->SetPointCoordinates(0, trialPoints[counter]);
        counter++;

        for( int i = 0; i < m->GetNumberOfPoints(); ++i )  {
             double * mpos = m->GetPointCoordinates(i);
                std::cout<< mpos[0] << mpos[1] << mpos[2];
                m_pointCoord[0] = mpos[0];
                m_pointCoord[1] = mpos[1];
                m_pointCoord[2] = mpos[2];
        }

        //Calculate Distance
        double distanceToTarget = sqrt(pow((m_pointCoord[0] - pos[0]),2) + pow((m_pointCoord[1] - pos[1]),2) +
                pow((m_pointCoord[3] - pos[3]),2));

        double distanceToStartPoint = sqrt(pow((m_pointCoord[0] - 200),2) + pow((m_pointCoord[1] - 200),2) +
                pow((m_pointCoord[3] - 200),2));


        //Trigger End Trial if Within Range
        if (distanceToTarget < 10.0){
            std::cout<< "Within threshold; End Trial";
            char buffer[OUTPUT_BUFFER_SIZE];
            osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
            p << osc::BeginBundleImmediate
            << osc::BeginMessage( "/endTrialSignal" ) << "bang" << osc::EndMessage << osc::EndBundle;
            transmitSocket.Send( p.Data(), p.Size() );
        }

        //Trigger Start Trial if Within Range of Marker
        if (distanceToStartPoint < 10.0){
            std::cout<< "Within threshold; Start Trial";
            char buffer[OUTPUT_BUFFER_SIZE];
            osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
            p << osc::BeginBundleImmediate
            << osc::BeginMessage( "/beginTrialSignal" ) << "bang" << osc::EndMessage << osc::EndBundle;
            transmitSocket.Send( p.Data(), p.Size() );
        }

        //Send Pointer Information
        char buffer[OUTPUT_BUFFER_SIZE];
        osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );        
        p << osc::BeginBundleImmediate
            << osc::BeginMessage( "/pointerX" ) << ((float)m_tipPosition[0]) << osc::EndMessage
            << osc::BeginMessage( "/pointerY" ) << ((float)m_tipPosition[1]) << osc::EndMessage
            << osc::BeginMessage( "/pointerZ" ) << ((float)m_tipPosition[2]) << osc::EndMessage
            << osc::BeginMessage( "/pointerState" ) << p_state << osc::EndMessage
        << osc::EndBundle;
        transmitSocket.Send( p.Data(), p.Size() );
        
        emit Modified();
    }
}

void OSCCapturePluginInterface::OnPointsModified() //Currently Not Used
{
}

bool OSCCapturePluginInterface::HandleKeyboardEvent( QKeyEvent * keyEvent )
{
    if( keyEvent -> key() == Qt::Key_Space ){
        PointsObject * p = PointsObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_pointsId ) );
        Q_ASSERT( p );

        p->SetPointCoordinates(0, trialPoints[counter]);
        counter++;

        for( int i = 0; i < p->GetNumberOfPoints(); ++i )  {
            double * pos = p->GetPointCoordinates( i );
            char buffer[OUTPUT_BUFFER_SIZE];
            osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

            p << osc::BeginBundleImmediate
                << osc::BeginMessage( "/pointMarkerX" ) << ((float)pos[0]) << osc::EndMessage
                << osc::BeginMessage( "/pointMarkerY" ) << ((float)pos[1]) << osc::EndMessage
                << osc::BeginMessage( "/pointMarkerZ" ) << ((float)pos[2]) << osc::EndMessage
                << osc::BeginMessage( "/beginTrialSignal" )  << "bang" << osc::EndMessage
              << osc::EndBundle;

        transmitSocket.Send( p.Data(), p.Size() );
        }
        return true;
    }
    else if (keyEvent -> key() == Qt::Key_1){
        char buffer[OUTPUT_BUFFER_SIZE];
        osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
        p << osc::BeginBundleImmediate
            << osc::BeginMessage( "/endTrialSignal" ) << "bang" << osc::EndMessage << osc::EndBundle;
        transmitSocket.Send( p.Data(), p.Size() );
         return true;
    }
    return false;
}

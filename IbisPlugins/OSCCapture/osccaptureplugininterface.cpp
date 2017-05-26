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
#include "view.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "osc/OscOutboundPacketStream.h"    //Import OSCpack Library
#include "ip/UdpSocket.h"                   //Import OSCpack Library
#include <QKeyEvent>

#define ADDRESS "127.0.0.1"                 //OSC Data Address
#define PORT 8005                           //OSC Data Port
#define OUTPUT_BUFFER_SIZE 1024             //OSC Data Size
UdpTransmitSocket transmitSocket( IpEndpointName( ADDRESS, PORT ) );
int counter = 0;

double testPoints[3][3] = {
{-20.8889, -33.2248, -532.156},
{2.8889, -3.2248, -2.156},
{-15.8889, -40.2248, -132.156},
};


double trialPoints[50][3] = {
{-2.8889, -33.2248, -532.156},
{-2.8889, -40.637, -489.906},
{-1.1689, -28.0362, -534.38},
{-7.6189, -10.2468, -481.753 },
{-7.1889, 37.1915, -517.332 },
{0.551104, -67.3211, -503.99},
{-6.7589, 23.8495, -502.507},
{-5.4689, 13.4723, -509.178},
{-1.40645, -34.7072, -530.674},
{-47.8626, -11.3376, -529.191},
{46.0319, -11.3376, -528.45},
{28.2425, -3.16765, -485.459},
{2.01251, -68.0623, -557.358},
{-12.5248, -42.6923, -539.568},
{31.7497, -23.3223, -517.568},
{47.2297, -62.1325, -555.134},
{-7.33624, 20.8846, -501.025},
{28.9837, 20.8846, -552.916},
{-60.7044, 0.130308, -555.134},
{-39.9501, 0.130308, -529.932},
{-5.85379, -9.32969, -479.529},
{-62.4154, -18.186, -528.45},
{26.7601, -35.816, -550.687},
{33.4618, 28.0405, -538.086},
{-7.33624, -82.1456, -505.472},
{-3.34819, -10.4816, -573.665},
{-3.34819, 4.92725, -507.696},
{36.886, -2.77718, -519.696},
{-44.4385, -25.0344, -562.696},
{-1.40645, -73.1549, -489.165},
{18.6066, -38.7312, -549.204},
{52.1466, 11.2487, -535.121},
{0.976599, 26.0731, -507.696},
{-10.2034, -41.3782, -537.345},
{10.0066, -35.4484, -540.31},
{-6.3334, 22.367, -503.248},
{-38.4676, 19.357, -538.827},
{-47.3623, 5.59703, -541.051},
{-19.1958, -16.333, -551.428},
{-7.33624, -28.373, -463.963},
{-59.2219, -28.373, -525.485},
{48.9968, -33.533, -531.415},
{30.0377, 16.0559, -543.274},
{14.1277, -66.5799, -555.875},
{6.54181, -31.7423, -536.603},
{5.21228, 7.49539, -556.603},
{-35.2077, -10.9881, -495.836},
{-7.62843, 70.8429, -538.086},
{-9.77843, 63.1343, -501.025},
{-56.9982, -3.57581, -513.625}
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

    vtkCamera * cam = GetSceneManager()->GetMain3DView()->GetRenderer()->GetActiveCamera();
    Q_ASSERT( cam );

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
        PointsObject * m = PointsObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_pointsId ) );
        Q_ASSERT( m );
        double * pointCoord = m->GetPointCoordinates( 0 );

//        //Calculate Distance (Watch for infinity errors)
//        float distanceToTarget = sqrt(pow((pointCoord[0] - pos[0]),2) + pow((pointCoord[1] - pos[1]),2) +
//                pow((pointCoord[3] - pos[3]),2));

        float distanceToStartPoint = sqrt(pow((pos[0] - 125.0),2) + pow((pos[1] - 150.0),2) +
                pow((pos[3] - -35.0),2));


//        //Trigger End Trial if Within Range
//        if (distanceToTarget < 10.0){
//            std::cout<< "Within threshold; End Trial";
//            char buffer[OUTPUT_BUFFER_SIZE];
//            osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
//            p << osc::BeginBundleImmediate
//            << osc::BeginMessage( "/endTrialSignal" ) << "bang" << osc::EndMessage << osc::EndBundle;
//            transmitSocket.Send( p.Data(), p.Size() );
//        }

        //Trigger Start Trial if Within Range of Marker
        if (distanceToStartPoint < 50.0){
            std::cout<< "Within threshold; Start Trial";
//            char buffer[OUTPUT_BUFFER_SIZE];
//            osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
//            p << osc::BeginBundleImmediate
//            << osc::BeginMessage( "/beginTrialSignal" ) << "bang" << osc::EndMessage << osc::EndBundle;
//            transmitSocket.Send( p.Data(), p.Size() );

            //Cue the start trial function
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

            //Code for altering view on each trail
            vtkCamera * cam = GetSceneManager()->GetMain3DView()->GetRenderer()->GetActiveCamera();
            Q_ASSERT( cam );

            //SetPosition( x, y, z )    // position of the optical center, were everything is projected
            cam->SetPosition(testPoints[counter%3]);


        }

        //Send Pointer Information
        char buffer[OUTPUT_BUFFER_SIZE];
        osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );        
        p << osc::BeginBundleImmediate
            << osc::BeginMessage( "/pointerX" ) << ((float)m_tipPosition[0]) << osc::EndMessage
            << osc::BeginMessage( "/pointerY" ) << ((float)m_tipPosition[1]) << osc::EndMessage
            << osc::BeginMessage( "/pointerZ" ) << ((float)m_tipPosition[2]) << osc::EndMessage
            << osc::BeginMessage( "/pointerState" ) << p_state << osc::EndMessage

//            << osc::BeginMessage( "/distance" ) << ((float)distanceToTarget) << osc::EndMessage
            << osc::BeginMessage( "/reset" ) << ((float)distanceToStartPoint) << osc::EndMessage

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

        //Code for altering view on each trail
        vtkCamera * cam = GetSceneManager()->GetMain3DView()->GetRenderer()->GetActiveCamera();
        Q_ASSERT( cam );

        //SetPosition( x, y, z )    // position of the optical center, were everything is projected
        cam->SetPosition(testPoints[counter%3]);

        //SetFocalPoint( x, y, z )  // Where the camera is looking, the target
        //cam->SetFocalPoint(testPoints[counter%3]);

        //SetViewUp( x, y, z )    // up of the camera: allows to roll the camera around its optical axis.
        //cam->SetViewUp(testPoints[counter%3]);

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

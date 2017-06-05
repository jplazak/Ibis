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
#include <ctime>
#include <QTime>
#include <algorithm>    // std::shuffle
#include <array>        // std::array
#include <random>       // std::default_random_engine
#include <chrono>       // std::chrono::system_clock
#include <iostream>     // std::cout
#include "imageobject.h"
#include <vtkMatrix4x4.h>
#include <vtkSmartPointer.h>
#include <vtkPerspectiveTransform.h>
#include <vtkTransform.h>
#include "vtkLinearTransform.h"

#define ADDRESS "127.0.0.1"                 //OSC Data Address
#define PORT 8005                           //OSC Data Port
#define OUTPUT_BUFFER_SIZE 1024             //OSC Data Size
UdpTransmitSocket transmitSocket( IpEndpointName( ADDRESS, PORT ) );
int counter = 0;
bool trialReady = 1;

QTime t1;
QTime t2;
//QList< ImageObject* > allObjects;


//// sonfication type (0-4 are sounds, 5 is silent)
std::array<int,30> trial = {
0,0,1,1,2,2,3,3,4,4,    //Audio (all need to be paired with NULL VIEW)
5,5,5,5,5,5,5,5,5,5,    //Visual (Random view & no sound)
6,6,7,7,8,8,9,9,10,10    //AudioVisual (Random View)
};



double testPoints[10][3] = {
{-170.8889, -13.2248, -518.6},  //1
{-170.8889, -13.2248, -518.6},  //4 move y by a small amount
{-170.8889, -13.2248, -508.6},  //7move y by a small amount
{-160.8889, -13.2248, -518.6},  //2
{-160.8889, -15.2248, -518.6},  //5
{-160.8889, -15.2248, -503.6},  //8
{-150.8889, -13.2248, -518.6},  //3  x should be between -150 & -170
{-150.8889, -11.2248, -518.6},  //6
{-150.8889, -11.2248, -513.6},   //9
{-1500.8889, -1100.2248, -5130.6},   //10need one point where everything is off the screen
};


int sonificationCode = 0;

double trialPoints[50][3] = {
{-2.8889, -33.2248, -32.156},
{-2.8889, -40.637, 11.906},
{-1.1689, -28.0362, -34.38},
{-7.6189, -10.2468, -18.247},
{-7.1889, 37.1915, -17.332 },
{0.551104, -67.3211, -03.99},
{-6.7589, 23.8495, -02.507},
{-5.4689, 13.4723, -09.178},
{-1.40645, -34.7072, -30.674},
{-47.8626, -11.3376, -29.191},
{46.0319, -11.3376, -28.45},
{28.2425, -3.16765, 14.541},
{2.01251, -68.0623, -57.358},
{-12.5248, -42.6923, -39.568},
{31.7497, -23.3223, -17.568},
{47.2297, -62.1325, -55.134},
{-7.33624, 20.8846, -01.025},
{28.9837, 20.8846, -52.916},
{-60.7044, 0.130308, -55.134},
{-39.9501, 0.130308, -29.932},
{-5.85379, -9.32969, 20.471},
{-62.4154, -18.186, -28.45},
{26.7601, -35.816, -50.687},
{33.4618, 28.0405, -38.086},
{-7.33624, -82.1456, -05.472},
{-3.34819, -10.4816, -73.665},
{-3.34819, 4.92725, -07.696},
{36.886, -2.77718, -19.696},
{-44.4385, -25.0344, -62.696},
{-1.40645, -73.1549, 10.835},
{18.6066, -38.7312, -49.204},
{52.1466, 11.2487, -35.121},
{0.976599, 26.0731, -07.696},
{-10.2034, -41.3782, -37.345},
{10.0066, -35.4484, -40.31},
{-6.3334, 22.367, -03.248},
{-38.4676, 19.357, -38.827},
{-47.3623, 5.59703, -41.051},
{-19.1958, -16.333, -51.428},
{-7.33624, -28.373, 36.037},
{-59.2219, -28.373, -25.485},
{48.9968, -33.533, -31.415},
{30.0377, 16.0559, -43.274},
{14.1277, -66.5799, -55.875},
{6.54181, -31.7423, -36.603},
{5.21228, 7.49539, -56.603},
{-35.2077, -10.9881, 4.164},
{-7.62843, 70.8429, -38.086},
{-9.77843, 63.1343, -01.025},
{-56.9982, -3.57581, -13.625}
};

OSCCapturePluginInterface::OSCCapturePluginInterface()
{
    m_tipPosition[0] = 0.0;
    m_tipPosition[1] = 0.0;
    m_tipPosition[2] = 0.0;
    m_pointsId = SceneManager::InvalidId;
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
        p->SetCanChangeParent( true );      //was false
        p->SetCanAppendChildren( true);   //was false
        p->SetCanEditTransformManually( true );    //was false
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

    //Shuffle Trials
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(trial.begin(), trial.end(), std::default_random_engine(seed));

    std::cout << "First trial will be: " << trial[0] << std::endl;

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
        if (distanceToStartPoint < 38.0 && trialReady){
            t1 = QTime::currentTime();
            trialReady = false;

           //Start trial function
            PointsObject * p = PointsObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_pointsId ) );
            Q_ASSERT( p );

            p->SetPointCoordinates(0, trialPoints[counter%50]);
            counter++;

            sonificationCode = trial[ (counter % 30) ];
            //sonificationCode = ((sonificationCode + 1) % 6);
            std::cout << "Sonification code for trial #" << counter << " is: " << sonificationCode << std::endl;

            for( int i = 0; i < p->GetNumberOfPoints(); ++i )  {
                double * pos = p->GetPointCoordinates( i );
                char buffer[OUTPUT_BUFFER_SIZE];
                osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );


                p << osc::BeginBundleImmediate
                    << osc::BeginMessage( "/pointMarkerX" ) << ((float)pos[0]) << osc::EndMessage
                    << osc::BeginMessage( "/pointMarkerY" ) << ((float)pos[1]) << osc::EndMessage
                    << osc::BeginMessage( "/pointMarkerZ" ) << ((float)pos[2]) << osc::EndMessage
                    << osc::BeginMessage( "/sonification" ) << sonificationCode << osc::EndMessage
                    << osc::BeginMessage( "/beginTrialSignal" )  << "bang" << osc::EndMessage
                  << osc::EndBundle;

            transmitSocket.Send( p.Data(), p.Size() );
            }

            //Code for altering view on each trail
            vtkCamera * cam = GetSceneManager()->GetMain3DView()->GetRenderer()->GetActiveCamera();
            Q_ASSERT( cam );

            //SetPosition( x, y, z )    // position of the optical center, were everything is projected
            if (sonificationCode < 5){
                std::cout << "Test Point off screen" << endl;
               // cam->SetPosition(testPoints[9]);
            } else {
              // cam->SetPosition(testPoints[counter%9]);
            }


        }

        //Check Timer
        t2 = QTime::currentTime();
        qint64 msDifference = t1.msecsTo(t2);
        if (msDifference > 16000){
            trialReady = true;
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
    QList< ImageObject* > allObjects;
    GetSceneManager()->GetAllImageObjects( allObjects );  // sceneManager fills the list with all images object int the scene
    bool test = allObjects.size()  == 1;
    Q_ASSERT( test ); // make sure there is one and only one images object.
    ImageObject * wantedObject = allObjects[0];  // just get the first one


    if( keyEvent -> key() == Qt::Key_Space ){
        PointsObject * p = PointsObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_pointsId ) );
        Q_ASSERT( p );

        p->SetPointCoordinates(0, trialPoints[counter%50]);
        counter++;

        sonificationCode = trial[ (counter % 30) ];
        std::cout << "Sonification code for trial #" << counter << " is: " << sonificationCode << std::endl;

        //Code for altering view on each trail
        vtkCamera * cam = GetSceneManager()->GetMain3DView()->GetRenderer()->GetActiveCamera();
        Q_ASSERT( cam );


        //SetPosition( x, y, z )    // position of the optical center, were everything is projected
      //  cam->SetPosition(testPoints[counter%9]);

        //SetFocalPoint( x, y, z )  // Where the camera is looking, the target
        //cam->SetFocalPoint(-130.8889, -13.2248, -518.6);
        cam->SetFocalPoint(100, 100, -130);

        //SetViewUp( x, y, z )    // up of the camera: allows to roll the camera around its optical axis.
        //cam->SetViewUp(testPoints[counter%3]);
       // cam->SetViewUp(0,0,1);



        vtkTransform * transform = vtkTransform::New();
        transform->RotateX(double(counter * 10));
        transform->RotateY(double(counter * 20));
        transform->RotateZ(double(counter * 30));
        wantedObject->SetLocalTransform(transform);
        transform->Delete();

//        vtkLinearTransform * localPosition = wantedObject->GetLocalTransform();
//        std::cout << *localPosition <<std::endl;

//        PointsObject * p2 = PointsObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_pointsId ) );
//        p2->SetLocalTransform(transform);
//            double * pos2 = p2->GetPointCoordinates(0);
//            std::cout << pos2[0] << pos2[1] << pos2[2] << std::endl;
//            p2->

  //      transform->Delete();

//        vtkTransform * transform3 = vtkTransform::New();
//        transform3->RotateY(double(counter *10));
//       transform3->RotateWXYZ((counter*20.0),1.0,0.5,0.25);
//        wantedObject->SetLocalTransform(transform3);
//        transform3->Delete();

        vtkTransform * tp = p->GetWorldTransform();
        double original[4] = { 0.0, 0.0, 0.0, 1.0 };
        Q_ASSERT( p->GetNumberOfPoints() == 1 );
        double * pos = p->GetPointCoordinates( 0 );
        original[0] = pos[0];
        original[1] = pos[1];
        original[2] = pos[2];
        double transformedPoint[4] = { 0.0, 0.0, 0.0, 1.0 };
        tp->TransformPoint( pos, transformedPoint );
        std::cout <<"World Coordinates: " << transformedPoint[0] << " " << transformedPoint[1]<< " " << transformedPoint[2] << " " << std::endl;

        for( int i = 0; i < p->GetNumberOfPoints(); ++i )  {
            double * pos = p->GetPointCoordinates( i );
            //std::cout << pos[0] << pos[1] << pos[2] << std::endl;
            char buffer[OUTPUT_BUFFER_SIZE];
            osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

            p << osc::BeginBundleImmediate
//                << osc::BeginMessage( "/pointMarkerX" ) << ((float)pos[0]) << osc::EndMessage
//                << osc::BeginMessage( "/pointMarkerY" ) << ((float)pos[1]) << osc::EndMessage
//                << osc::BeginMessage( "/pointMarkerZ" ) << ((float)pos[2]) << osc::EndMessage

                << osc::BeginMessage( "/pointMarkerX" ) << ((float)transformedPoint[0]) << osc::EndMessage
                << osc::BeginMessage( "/pointMarkerY" ) << ((float)transformedPoint[1]) << osc::EndMessage
                << osc::BeginMessage( "/pointMarkerZ" ) << ((float)transformedPoint[2]) << osc::EndMessage

                << osc::BeginMessage( "/viewpoint" ) << (counter%9) << osc::EndMessage
                << osc::BeginMessage( "/beginTrialSignal" )  << "bang" << osc::EndMessage
              << osc::EndBundle;

        transmitSocket.Send( p.Data(), p.Size() );
        }


        return true;
    }
    else if (keyEvent -> key() == Qt::Key_1){
//        char buffer[OUTPUT_BUFFER_SIZE];
//        osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
//        p << osc::BeginBundleImmediate
//            << osc::BeginMessage( "/endTrialSignal" ) << "bang" << osc::EndMessage << osc::EndBundle;
//        transmitSocket.Send( p.Data(), p.Size() );

         vtkTransform * transform2 = vtkTransform::New();
         //transform2->RotateY(50.0);
         //transform2->RotateX(0.0);
         //transform2->Translate(10.0,10.0,10.0);
           transform2->RotateY(double(counter));
         wantedObject->SetLocalTransform(transform2);

         transform2->Delete();

         return true;
    }
    else if (keyEvent -> key() == Qt::Key_2){
         vtkTransform * transform3 = vtkTransform::New();
         transform3->RotateZ(double(counter));
         wantedObject->SetLocalTransform(transform3);
         transform3->Delete();

         return true;
    }



    return false;
}

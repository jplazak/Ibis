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

#ifndef __VolumeRenderingObject_h_
#define __VolumeRenderingObject_h_

#include <map>
#include "sceneobject.h"
#include "shadercontrib.h"

class ImageObject;
class View;
class vtkVolume;
class vtkVolumeProperty;
class vtkImageShiftScale;
class vtkPRISMVolumeMapper;
class vtkPiecewiseFunction;
class vtkEventQtSlotConnect;
class vtkHandleWidget;
class vtkLineWidget2;
class vtkIbisRenderState;

class VolumeRenderingObject : public SceneObject
{

    Q_OBJECT

public:

    static VolumeRenderingObject * New() { return new VolumeRenderingObject; }
    vtkTypeMacro(VolumeRenderingObject,SceneObject);

    VolumeRenderingObject();
    virtual ~VolumeRenderingObject();

    virtual void Serialize( Serializer * ser );
    void Clear();

    // SceneObject implementation
    virtual void Setup( View * view );
    virtual void PreDisplaySetup();
    virtual void Release( View * view );
    virtual QWidget * CreateSettingsDialog( QWidget * parent );
    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets) {}

    // Use picking to set interaction point
    bool Pick( View * v, int x, int y, double pickedPos[3] );
    virtual bool OnLeftButtonPressed( View * v, int x, int y, unsigned modifiers );
    bool GetPickPos() { return m_pickPos; }
    void SetPickPos( bool set );
    double GetPickValue() { return m_pickValue; }
    void SetPickValue( double val );

    // Implement Volume render style. simtodo : move to a separate class (VolumeRenderingStyle)
    ImageObject * GetImage( int slotIndex );
    void SetImage( int slotIndex, ImageObject * im );
    int GetNumberOfImageSlots();
    void AddImageSlot();
    void RemoveImageSlot();

    bool IsVolumeEnabled( int index );
    void EnableVolume( int index, bool enable );

    void Animate( bool on );
    bool IsAnimating() { return m_isAnimating; }

    void SetMultFactor( double val );
    double GetMultFactor() { return m_multFactor; }

    void SetSamplingDistance( double samplingDistance );
    double GetSamplingDistance() { return m_samplingDistance; }

    // Manage set of points that are passes to the GPU shader to allow interaction with the volume
    void SetShowInteractionWidget( bool s );
    bool GetShowInteractionWidget() { return m_showInteractionWidget; }
    void SetPointerTracksInteractionPoints( bool t );
    bool GetPointerTracksInteractionPoints() { return m_pointerTracksInteractionPoints; }
    void SetInteractionWidgetAsLine( bool l );
    bool IsInteractionWidgetLine() { return m_interactionWidgetLine; }
    void SetInteractionPoint1( double x, double y, double z );
    void GetInteractionPoint1( double & x, double & y, double & z );
    void SetInteractionPoint2( double x, double y, double z );
    void GetInteractionPoint2( double & x, double & y, double & z );

    // Manage ray init shaders
    int GetNumberOfRayInitShaderTypes();
    QString GetRayInitShaderTypeName( int index );
    bool IsRayInitShaderTypeCustom( int index );
    bool DoesRayInitShaderExist( QString name );
    void SetRayInitShaderType( int typeIndex );
    void SetRayInitShaderTypeByName( QString name );
    int GetRayInitShaderType();
    QString GetRayInitShaderName();
    void AddRayInitShaderType( QString name, QString code, bool custom );
    void DuplicateRayInitShaderType();
    void DeleteRayInitShaderType();
    QString GetRayInitShaderCode();
    void SetRayInitShaderCode( QString code );

    // Manage Stop condition shaders
    int GetNumberOfStopConditionShaderTypes();
    QString GetStopConditionShaderTypeName( int index );
    bool IsStopConditionShaderTypeCustom( int index );
    bool DoesStopConditionShaderTypeExist( QString name );
    void SetStopConditionShaderType( int typeIndex );
    void SetStopConditionShaderTypeByName( QString name );
    int GetStopConditionShaderType();
    QString GetStopConditionShaderName();
    void AddStopConditionShaderType( QString name, QString code, bool custom );
    void DuplicateStopConditionShaderType();
    void DeleteStopConditionShaderType();
    QString GetStopConditionShaderCode();
    void SetStopConditionShaderCode( QString code );

    // Manage shader contribution of each volume
    int GetNumberOfShaderContributionTypes();
    int GetShaderContributionTypeIndex( QString shaderName );
    QString GetShaderContributionTypeName( int typeIndex );
    bool DoesShaderContributionTypeExist( QString name );
    void AddShaderContributionType( QString shaderName, QString code, bool custom );
    QString GetUniqueCustomShaderName( QString name );
    int DuplicateShaderContribType( int typeIndex );
    void DeleteShaderContributionType( int typeIndex );
    void SetShaderContributionType( int volumeIndex, int typeIndex );
    void SetShaderContributionTypeByName( int volumeIndex, QString name );
    int GetShaderContributionType( int volumeIndex );
    bool IsShaderTypeCustom( int typeIndex );
    QString GetCustomShaderContribution( int volumeIndex );
    void SetCustomShaderCode( int volumeIndex, QString code );

    void SetUse16BitsVolume( int volumeIndex, bool use );
    bool GetUse16BitsVolume( int volumeIndex );
    void SetUseLinearSampling( int volumeIndex, bool isLinear );
    bool GetUseLinearSampling( int volumeIndex );

    vtkVolumeProperty * GetVolumeProperty( int index );

    void SetRenderState( vtkIbisRenderState * state );

    struct PerImage
    {
        PerImage();
        ~PerImage();
        void Serialize( Serializer * ser );
        int lastImageObjectId;  // for serialization
        ImageObject * image;
        vtkImageShiftScale * imageCast;
        vtkVolumeProperty * volumeProperty;
        bool volumeEnabled;
        bool volumeIs16Bits;
        bool linearSampling;
        int shaderContributionType;
        QString shaderContributionTypeName;
    };

    void SaveCustomShaders();
    void SaveCustomShaders( QString baseDirectory );
    void LoadCustomShaders();
    void ImportCustomShaders( QString baseDirectory, QString & initName, QString & stopName );

signals:

    void VolumeSlotModified();

public slots:

    void TransferFunctionModifiedSlot();
    void ObjectAddedSlot( int objectId );
    void ObjectRemovedSlot( int objectId );
    void TrackingUpdatedSlot();
    void InteractionWidgetMoved( vtkObject * caller );

protected:

    virtual void Hide();
    virtual void Show();
    void ObjectAddedToScene();
    void ObjectRemovedFromScene();

    void UpdateInteractionWidgetVisibility();
    void UpdateInteractionPointPos();
    virtual void timerEvent( QTimerEvent * e );
    struct PerView;
    void UpdateMapper( PerView & pv );
    void UpdateAllMappers();
    void UpdateShiftScale( int index );
    void UpdateShaderParams();
    void ConnectImageSlot( PerImage * pi );

    virtual void InternalPostSceneRead();

    QList< ShaderContrib > m_volumeShaders;
    void UpdateShaderContributionTypeIndices();

    QList< ShaderContrib > m_initShaders;
    int m_initShaderContributionType;

    QList< ShaderContrib > m_stopConditionShaders;
    int m_stopConditionShaderType;

    struct PerView
    {
        vtkPRISMVolumeMapper * volumeMapper;
        vtkVolume * volumeActor;
        vtkHandleWidget * sphereWidget;
        vtkLineWidget2 * lineWidget;
    };
    typedef std::map< View*, PerView > PerViewContainer;
    PerViewContainer m_perView;


    typedef std::vector< PerImage * > PerImageContainer;
    PerImageContainer m_perImage;

    vtkEventQtSlotConnect * m_transferFunctionModifiedCallback;

    // For animation
    QTime * m_time;
    int m_timerId;

    // Manage points used for interaction
    bool m_showInteractionWidget;
    bool m_pointerTracksInteractionPoints;
    bool m_interactionWidgetLine;
    vtkEventQtSlotConnect * m_interactionWidgetModifiedCallback;
    double m_interactionPoint1[3];
    double m_interactionPoint2[3];
    bool m_pickPos;
    double m_pickValue;

    bool m_isAnimating;
    double m_multFactor;
    double m_samplingDistance;
};

ObjectSerializationHeaderMacro( VolumeRenderingObject );
ObjectSerializationHeaderMacro( VolumeRenderingObject::PerImage );

#endif

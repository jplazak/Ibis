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

#include "vtkIbisImagePlaneMapper.h"
#include "vtkInformation.h"
#include "vtkImageData.h"
#include "vtkExecutive.h"
#include "vtkgl.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "GlslShader.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkRenderer.h"
#include "vtkSimpleProp3D.h"
#include "vtkMatrix4x4.h"

vtkStandardNewMacro(vtkIbisImagePlaneMapper);

vtkIbisImagePlaneMapper::vtkIbisImagePlaneMapper()
{
    this->LastInput = 0;
    this->TextureId = 0;
    this->Shader = 0;
    this->OpenGLExtensionsLoaded = false;
    this->GlobalOpacity = 1.0;
    this->ImageCenter[ 0 ] = 319.5;
    this->ImageCenter[ 1 ] = 239.5;
    this->LensDistortion = 0.0;
    this->UseTransparency = true;
    this->UseGradient = true;
    this->ShowMask = false;
    this->TransparencyPosition[0] = 320.0;
    this->TransparencyPosition[1] = 240.0;
    this->TransparencyRadius[0] = 20.0;
    this->TransparencyRadius[1] = 200.0;
    this->Saturation = 1.0;
    this->Brightness = 1.0;
    this->SetNumberOfInputPorts(1);
}

vtkIbisImagePlaneMapper::~vtkIbisImagePlaneMapper()
{  
}

int vtkIbisImagePlaneMapper::RenderTranslucentPolygonalGeometry( vtkRenderer *ren, vtkSimpleProp3D * prop )
{
    // Test if OpenGL has all extensions needed and if they are loaded
    if(!this->OpenGLExtensionsLoaded)
        this->LoadOpenGLExtensions( ren->GetRenderWindow() );
    if(!this->OpenGLExtensionsLoaded)
        return 0;

    // Put input image in OpenGL texture
    this->UpdateTexture();

    // Update OpenGL shader
    if( !this->UpdateShader() )
        return 0;

    // Get input image
    vtkImageData * input = vtkImageData::SafeDownCast( this->GetInput() );
    if( !input )
        return 0;

    // OpenGL matrices are column-order, not row-order like VTK
    if( prop )
    {
        vtkMatrix4x4 * matrix = prop->GetMatrix();
        double mat[16];
        vtkMatrix4x4::Transpose(*matrix->Element, mat);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glMultMatrixd(mat);
    }

    // mainly to remember blendind state
    glPushAttrib( GL_COLOR_BUFFER_BIT );

    // Draw the image
    int * dim = input->GetDimensions();
    double * origin = input->GetOrigin();
    double * step = input->GetSpacing();

    double minX = origin[0];
    double maxX = origin[0] + ( dim[0] - 1 ) * step[0];
    double minY = origin[1];
    double maxY = origin[1] + ( dim[1] - 1 ) * step[1];

    double offsetX = this->ImageCenter[0] - ( dim[0] * 0.5 - 0.5 );
    double offsetY = this->ImageCenter[1] - ( dim[1] * 0.5 - 0.5 );

    this->Shader->UseProgram( true );
    this->Shader->SetVariable( "mainTexture", int(0) );
    this->Shader->SetVariable( "UseTransparency", this->UseTransparency );
    this->Shader->SetVariable( "UseGradient", this->UseGradient );
    this->Shader->SetVariable( "ShowMask", this->ShowMask );
    float transpPosX = (float)(this->TransparencyPosition[0] * dim[0]);
    float transpPosY = (float)(this->TransparencyPosition[1] * dim[1]);
    this->Shader->SetVariable( "TransparencyPosition", transpPosX, transpPosY );
    float transpMin = (float)(this->TransparencyRadius[0] * dim[0]);
    float transpMax = (float)(this->TransparencyRadius[1] * dim[0]);
    this->Shader->SetVariable( "TransparencyRadius", transpMin, transpMax );
    this->Shader->SetVariable( "ImageOffset", (float)offsetX, (float)offsetY );
    this->Shader->SetVariable( "ImageCenter", (float)ImageCenter[0], (float)ImageCenter[1] );
    this->Shader->SetVariable( "LensDistortion", (float)this->LensDistortion );
    this->Shader->SetVariable( "GlobalOpacity", (float)(this->GlobalOpacity) );
    this->Shader->SetVariable( "Saturation", (float)(this->Saturation) );
    this->Shader->SetVariable( "Brightness", (float)(this->Brightness) );

    glColor4d( 1.0, 1.0, 1.0, 1.0 );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( vtkgl::TEXTURE_RECTANGLE );
    glBindTexture( vtkgl::TEXTURE_RECTANGLE, this->TextureId );
    glBegin( GL_QUADS );
    {
        glTexCoord2d( 0.5, 0.5 );                   glVertex2d( minX, minY );
        glTexCoord2d( dim[0] - 0.5, 0.5 );          glVertex2d( maxX, minY );
        glTexCoord2d( dim[0] - 0.5, dim[1] - 0.5 );	glVertex2d( maxX, maxY );
        glTexCoord2d( 0.5, dim[1] - 0.5 );          glVertex2d( minX, maxY );
    }
    glEnd();
    glBindTexture( vtkgl::TEXTURE_RECTANGLE, 0 );
    glDisable( vtkgl::TEXTURE_RECTANGLE );

    this->Shader->UseProgram( false );

    glPopAttrib();

    if( prop )
    {
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }

    return 1;
}

void vtkIbisImagePlaneMapper::ReleaseGraphicsResources(vtkWindow *)
{
    if( this->TextureId != 0 )
        glDeleteTextures( 1, &(this->TextureId) );
    this->TextureId = 0;
}

unsigned long int vtkIbisImagePlaneMapper::GetRedrawMTime()
{
    unsigned long mTime = this->GetMTime();
    vtkImageData * input = vtkImageData::SafeDownCast( this->GetInput() );
    if( input )
    {
        unsigned long time = input->GetMTime();
        mTime = ( time > mTime ? time : mTime );
    }
    return mTime;
}

double * vtkIbisImagePlaneMapper::GetBounds()
{
    vtkImageData * input = this->GetInput();
    if ( !input )
        vtkMath::UninitializeBounds(this->Bounds);
    else
    {
        this->Update();
        input->GetBounds(this->Bounds);
    }
    return this->Bounds;
}

void vtkIbisImagePlaneMapper::SetGlobalOpacity( double opacity )
{
    this->GlobalOpacity = opacity;
    this->Modified();
}

void vtkIbisImagePlaneMapper::SetImageCenter( double x, double y )
{
    this->ImageCenter[0] = x;
    this->ImageCenter[1] = y;
    this->Modified();
}

void vtkIbisImagePlaneMapper::SetLensDistortion( double dist )
{
    this->LensDistortion = dist;
    this->Modified();
}

void vtkIbisImagePlaneMapper::SetUseTransparency( bool use )
{
    this->UseTransparency = use;
    this->Modified();
}

void vtkIbisImagePlaneMapper::SetUseGradient( bool use )
{
    this->UseGradient = use;
    this->Modified();
}

void vtkIbisImagePlaneMapper::SetShowMask( bool show )
{
    this->ShowMask = show;
    this->Modified();
}

void vtkIbisImagePlaneMapper::SetTransparencyPosition( double x, double y )
{
    this->TransparencyPosition[ 0 ] = x;
    this->TransparencyPosition[ 1 ] = y;
    this->Modified();
}

void vtkIbisImagePlaneMapper::SetTransparencyRadius( double min, double max )
{
    this->TransparencyRadius[0] = min;
    this->TransparencyRadius[1] = max;
    this->Modified();
}

void vtkIbisImagePlaneMapper::SetSaturation( double s )
{
    this->Saturation = s;
    this->Modified();
}

void vtkIbisImagePlaneMapper::SetBrightness( double b )
{
    this->Brightness = b;
    this->Modified();
}

//----------------------------------------------------------------------------
int vtkIbisImagePlaneMapper::FillInputPortInformation(int port, vtkInformation* info)
{
    info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData" );
    //info->Set( vtkAlgorithm::INPUT_IS_REPEATABLE(), 1 );
    return 1;
}

void vtkIbisImagePlaneMapper::UpdateTexture()
{
    vtkImageData * input = vtkImageData::SafeDownCast( this->GetInput() );

    // Check if we need an update
    if( this->TextureId != 0 && this->LastInput == input && ( input && input->GetMTime() < this->LastInputTimeStamp.GetMTime() ) )
        return;

    // Remember Last
    this->LastInput = input;
    this->LastInputTimeStamp.Modified();

    // Delete the texture if there is no input
    if( !input )
    {
        if( this->TextureId != 0 )
            glDeleteTextures( 1, &(this->TextureId) );
        this->TextureId = 0;
        return;
    }

    if( input->GetScalarType() != VTK_UNSIGNED_CHAR )
    {
        vtkErrorMacro(<<"Only unsigned char images are supported by this class.");
        return;
    }

    // find out format
    GLenum format = GL_RGB;
    GLint internalFormat = GL_RGB;
    if( input->GetNumberOfScalarComponents() == 1 )
    {
        format = GL_RED;
        internalFormat = GL_RED;
    }

    // Setup the texture
    int dim[3];
    input->GetDimensions(dim);
    glEnable( vtkgl::TEXTURE_RECTANGLE_ARB );
    if( this->TextureId == 0 )
        glGenTextures( 1, &(this->TextureId) );
    glBindTexture( vtkgl::TEXTURE_RECTANGLE_ARB, this->TextureId );
    glTexParameteri( vtkgl::TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( vtkgl::TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexImage2D( vtkgl::TEXTURE_RECTANGLE_ARB, 0, internalFormat, dim[0], dim[1], 0, format, GL_UNSIGNED_BYTE, input->GetScalarPointer() );
    glBindTexture( vtkgl::TEXTURE_RECTANGLE_ARB, 0 );
    glDisable( vtkgl::TEXTURE_RECTANGLE_ARB );

    if( glGetError() != GL_NO_ERROR )
    {
        vtkErrorMacro( "Error uploading 2D texture to GPU" );
        return;
    }
}

void vtkIbisImagePlaneMapper::LoadOpenGLExtensions( vtkRenderWindow * window )
{
    vtkOpenGLExtensionManager * extensions = vtkOpenGLExtensionManager::New();
    extensions->SetRenderWindow( window );

    if( !extensions->ExtensionSupported("GL_VERSION_2_0") )
    {
        this->OpenGLExtensionsLoaded = false;
        vtkErrorMacro(<<"No support for OpenGL 2.0");
    }
    else
    {
        extensions->LoadExtension( "GL_VERSION_1_2" );
        extensions->LoadExtension( "GL_VERSION_1_3" );
        extensions->LoadExtension( "GL_VERSION_2_0" );
        this->OpenGLExtensionsLoaded = true;
    }

    extensions->Delete();
}

const char defaultShaderCode[] = " \
        uniform sampler2DRect mainTexture; \
        uniform bool UseTransparency; \
        uniform bool UseGradient; \
        uniform bool ShowMask; \
        uniform vec2 TransparencyPosition; \
        uniform vec2 TransparencyRadius;  \
        uniform vec2 ImageOffset; \
        uniform vec2 ImageCenter; \
        uniform float LensDistortion; \
        uniform float GlobalOpacity; \
        uniform float Saturation; \
        uniform float Brightness; \
        \
        vec3 rgb2hsv(vec3 c) \
        { \
            vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0); \
            vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g)); \
            vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r)); \
            float d = q.x - min(q.w, q.y); \
            float e = 1.0e-10; \
            return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x); \
        } \
        \
        vec3 hsv2rgb(vec3 c) \
        { \
            vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0); \
            vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www); \
            return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y); \
        } \
        \
        void main() \
        { \
            vec2 offsetTexCoord = gl_TexCoord[0].st + ImageOffset; \
            vec2 dist = offsetTexCoord - ImageCenter; \
            float r2 = dist.x * dist.x + dist.y * dist.y; \
            vec2 distortionOffset = dist * LensDistortion * r2; \
            vec2 newTexCoord = offsetTexCoord + distortionOffset; \
            float factor = 0.0; \
            if( UseTransparency ) \
            { \
                float dist = length( gl_TexCoord[0].st - TransparencyPosition ); \
                if( dist < TransparencyRadius.y ) \
                { \
                    if( dist > TransparencyRadius.x ) \
                    { \
                        float ratio = ( dist - TransparencyRadius.x ) / ( TransparencyRadius.y - TransparencyRadius.x ); \
                        factor = exp(-(ratio*ratio)/0.25); \
                    } \
                    else \
                        factor = 1.0; \
                } \
            } \
            vec4 origColor = texture2DRect( mainTexture, newTexCoord ); \
            vec3 hsv = rgb2hsv( origColor.rgb ); \
            hsv.y *= Saturation; \
            hsv.z *= Brightness; \
            vec3 modColor = hsv2rgb( hsv ); \
            gl_FragColor.rgb = modColor; \
            gl_FragColor.a = 1.0; \
            float alpha = 1.0; \
            if( UseGradient ) \
            { \
                if( factor > 0.01 ) \
                { \
                    vec4 gx = texture2DRect( mainTexture, newTexCoord + vec2(-1.0,-1.0) ); \
                    gx -= texture2DRect( mainTexture, newTexCoord + vec2(1.0,1.0) ); \
                    gx += 2.0 * texture2DRect( mainTexture, newTexCoord + vec2(-1.0,0.0) ); \
                    gx -= 2.0 * texture2DRect( mainTexture, newTexCoord + vec2(1.0,0.0) ); \
                    gx += texture2DRect( mainTexture, newTexCoord + vec2(-1.0,-1.0) ); \
                    gx -= texture2DRect( mainTexture, newTexCoord + vec2(1.0,-1.0) ); \
                    vec4 gy = texture2DRect( mainTexture, newTexCoord + vec2(-1.0,1.0) ); \
                    gy -= texture2DRect( mainTexture, newTexCoord + vec2(-1.0,-1.0) ); \
                    gy += 2.0 * texture2DRect( mainTexture, newTexCoord + vec2(0.0,1.0) ); \
                    gy -= 2.0 * texture2DRect( mainTexture, newTexCoord + vec2(0.0,-1.0) ); \
                    gy += texture2DRect( mainTexture, newTexCoord + vec2(1.0,1.0) ); \
                    gy -= texture2DRect( mainTexture, newTexCoord + vec2(1.0,-1.0) ); \
                    vec2 gradAll = vec2( length( gx ), length( gy ) ); \
                    alpha = ( 1.0 - factor ) + factor * length( gradAll ); \
                } \
            } \
            else \
                alpha = 1.0 - factor; \
            if( ShowMask ) \
            { \
                gl_FragColor.rgb = alpha; \
                gl_FragColor.a = 1.0; \
            } \
            else \
            { \
                gl_FragColor.a = min( alpha, GlobalOpacity ); \
            } \
        }";

bool vtkIbisImagePlaneMapper::UpdateShader()
{
    bool result = true;
    if( !this->Shader )
    {
        this->Shader = new GlslShader;
        this->Shader->Reset();
        this->Shader->AddShaderMemSource( defaultShaderCode );
        result = this->Shader->Init();
    }
    return result;
}

vtkImageData * vtkIbisImagePlaneMapper::GetInput()
{
    return vtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

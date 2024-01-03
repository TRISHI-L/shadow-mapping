#version 330 core

#define NEAR_PLANE 1.0 //��Դλ�õ�������ľ���
#define LIGHT_WORLD_SIZE 0.05 //��������ϵ�����Դ�ߴ�
#define LIGHT_FRUSTUM_WIDTH 6.0 //��Դ��׶�Ŀ��
#define LIGHT_SIZE_UV (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH) //��Դ��shadow map����ϵ�µ�һ������
#define NUM_SAMPLES 8 //���������
#define BLOCKER_SEARCH_NUM_SAMPLES NUM_SAMPLES
#define PCF_NUM_SAMPLES NUM_SAMPLES
#define FILTER_SIZE 3
#define NUM_RINGS 10


out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

#define EPS 1e-3
#define PI 3.141592653589793
#define PI2 6.283185307179586
// ����-1��1֮��������
highp float rand_1to1(highp float x)
{
    return fract(sin(x)*10000.0);
}
// ����0��1֮��������
highp float rand_2to1(vec2 uv)
{
    const highp float a = 12.9898, b = 78.233, c = 43758.5453;
    highp float dt = dot(uv.xy, vec2(a,b)), sn = mod(dt, PI);
    return fract(sin(sn) * c);
}

vec2 poissonDisk[NUM_SAMPLES];
void poissonDiskSamples( const in vec2 randomSeed ) {

  float ANGLE_STEP = PI2 * float( NUM_RINGS ) / float( NUM_SAMPLES );
  float INV_NUM_SAMPLES = 1.0 / float( NUM_SAMPLES );

  float angle = rand_2to1( randomSeed ) * PI2;
  float radius = INV_NUM_SAMPLES;
  float radiusStep = radius;

  for( int i = 0; i < NUM_SAMPLES; i ++ ) {
    poissonDisk[i] = vec2( cos( angle ), sin( angle ) ) * pow( radius, 0.75 );
    radius += radiusStep;
    angle += ANGLE_STEP;
  }
}
void uniformDiskSamples( const in vec2 randomSeed ) {

  float randNum = rand_2to1(randomSeed);
  float sampleX = rand_1to1( randNum ) ;
  float sampleY = rand_1to1( sampleX ) ;

  float angle = sampleX * PI2;
  float radius = sqrt(sampleY);

  for( int i = 0; i < NUM_SAMPLES; i ++ ) {
    poissonDisk[i] = vec2( radius * cos(angle) , radius * sin(angle)  );

    sampleX = rand_1to1( sampleY ) ;
    sampleY = rand_1to1( sampleX ) ;

    angle = sampleX * PI2;
    radius = sqrt(sampleY);
  }
}

//�����ڵ���ƽ�����
float FindBlocker(sampler2D shadowMap, vec2 uv, float zReceiver) {
    //������shadow map��Ѱ��BlockerDepth�ķ�Χ 
    float searchWidth = LIGHT_SIZE_UV * (zReceiver - NEAR_PLANE) / zReceiver; 
    float blockerSum = 0.0; 
    int numBlockers = 0; 
    for( int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; i++ ) 
    {   
        vec2 sampleCoords = uv + poissonDisk[i] * searchWidth;
        float shadowMapDepth = texture(shadowMap, sampleCoords).r;
        if ( shadowMapDepth < zReceiver ) { 
               blockerSum += shadowMapDepth; 
               numBlockers++; 
        } 
    }
    //û�б��ڵ�
    if(numBlockers == 0) {
        return -1.0;
    }
    float avgBlockerDepth = blockerSum / float(numBlockers); 
    return avgBlockerDepth;
}

// PCF�˲���
float PCF_Filter(sampler2D shadowMap, vec2 uv, float zReceiver, float bias, float filterRadiusUV) 
{ 
    float sum = 0.0f; 
    for ( int i = 0; i < PCF_NUM_SAMPLES; i++ ) 
    {
        vec2 sampleCoords = uv + poissonDisk[i] * filterRadiusUV;
        float depth = texture(shadowMap, sampleCoords).r;
        sum += zReceiver - bias > depth ? 1.0 : 0.0; 
    }
    for(int i = 0; i < PCF_NUM_SAMPLES; i++)
    {
        vec2 sampleCoords = uv + -poissonDisk[i].yx * filterRadiusUV;
        float depth = texture(shadowMap, sampleCoords).r;
        sum += zReceiver - bias > depth ? 1.0 : 0.0;
    }
    return sum / (2.0 * float(PCF_NUM_SAMPLES));
}
//PCSS
float PCSS(vec4 fragPosLightSpace) {
    // ͸�ӳ���
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    vec2 uv = projCoords.xy;
    float zReceiver = projCoords.z;
    //���ɲ����õ���ά���ƫ��ֵ
    poissonDiskSamples(projCoords.xy);
    //uniformDiskSamples(projCoords.xy);
    // STEP 1: blocker search 
    float numBlockers = 0; 
    float avgBlockerDepth = FindBlocker(shadowMap, uv, zReceiver);
    if( avgBlockerDepth == -1.0 ) {
        return 0.0;
    }
    // STEP 2: penumbra size 
    float penumbraRatio = (zReceiver - avgBlockerDepth) / avgBlockerDepth; 
    float filterRadiusUV = penumbraRatio * LIGHT_SIZE_UV * NEAR_PLANE / zReceiver; 
    // STEP 3: filtering 
    float bias = 0.001;
    if(projCoords.z > 1.0) {
        return 0.0;
    } else {
        return PCF_Filter(shadowMap, uv, zReceiver, bias, filterRadiusUV);
    }
    
}
//PCF
float PCF(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    poissonDiskSamples(projCoords.xy);

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float bias = 0.001;
    //float bias = max(0.05 * (1.0 - dot(normalize(fs_in.Normal), normalize(lightPos - fs_in.FragPos))), 0.005);
    // PCF
    float shadow = 0.0;
    //���ر߳�
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0){
       shadow = 0.0;
    } else {
        shadow = PCF_Filter(shadowMap, projCoords.xy, projCoords.z, bias, FILTER_SIZE * texelSize.x);
    }
    return shadow;
}
void main()
{           
    // ��ȡ������ɫ�ͷ���
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(0.7);
    // ������
    vec3 ambient = 0.3 * lightColor;
    // ������
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // ���淴��
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    
    // ������Ӱ
    //float shadow = PCF(fs_in.FragPosLightSpace); 
    float shadow = PCSS(fs_in.FragPosLightSpace);
    // �������
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    FragColor = vec4(lighting, 1.0);
}
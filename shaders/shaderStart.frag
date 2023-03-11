#version 410 core
in vec3 fragPos;
in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

//lighting
uniform	vec3 lightDir;
uniform	vec3 lightColor;

//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

uniform vec3 viewPos;
uniform float fogDensity;
uniform float transp;

struct PointLight {    
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  
#define NR_POINT_LIGHTS 10  
uniform PointLight pointLights[NR_POINT_LIGHTS];

vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;
float shadow;


float computeShadow(){
	// perform perspective divide
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	
	// Transform to [0,1] range
	normalizedCoords = normalizedCoords * 0.5 + 0.5;

	// Get closest depth value from light's perspective
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
	
	// Get depth of current fragment from light's perspective
	float currentDepth = normalizedCoords.z;
	
	// Check whether current frag pos is in shadow
	//float bias = 0.005;
	//float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
	
	
	//transform normal
	vec3 normalEye = normalize(fNormal);	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDir);
	
	float diffuseFactor = dot(normalEye, -lightDirN);
	float bias = mix(0.0011,0.0,diffuseFactor);
	
	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
	
	if (currentDepth > 1)
		return 0;
	
	return shadow;
}
vec3 ambientPoints = vec3(0.0f,0.0f,0.0f);
vec3 diffusePoints = vec3(0.0f,0.0f,0.0f);
vec3 specularPoints = vec3(0.0f,0.0f,0.0f);

void CalcPointLight(PointLight light, vec3 normal)
{

	vec3 norm = fNormal;
    vec3 viewDir = normalize(viewPos - fragPos);
	
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(diffuseTexture, fTexCoords).rgb);
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(diffuseTexture, fTexCoords).rgb);
    vec3 specular = light.specular * spec * vec3(texture(specularTexture, fTexCoords).rgb);
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    //return (ambient + diffuse + specular);
	ambientPoints += ambient;
	diffusePoints += diffuse;
	specularPoints += specular;
} 

void computeLightComponents()
{		
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 normalEye = normalize(fNormal);	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDir);
	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
		
	//compute ambient light
	ambient = ambientStrength * lightColor;
	
	//compute diffuse light
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	//compute specular light
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;
}

float computeFog()
{
 //float fogDensity = 0.40f;
 float fragmentDistance = length(fPosEye);
 float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

 return clamp(fogFactor, 0.0f, 1.0f);
}


void main() 
{
	ambientPoints = vec3(0.0f,0.0f,0.0f);
    diffusePoints = vec3(0.0f,0.0f,0.0f);
    specularPoints = vec3(0.0f,0.0f,0.0f);
	
	computeLightComponents();
	
	 for(int i = 0; i < NR_POINT_LIGHTS; i++)
        CalcPointLight(pointLights[i], fNormal);
	
	
	//vec3 baseColor = vec3(0.9f, 0.35f, 0.0f);//orange
	
	ambient *= texture(diffuseTexture, fTexCoords).rgb;
	diffuse *= texture(diffuseTexture, fTexCoords).rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;
	
	// color calculation without shadow
	//vec3 color = min((ambient + diffuse) + specular, 1.0f);
    
	//add shadow
	shadow = computeShadow();
	
	ambient += ambientPoints;
	diffuse += diffusePoints;
	specular += specularPoints;
	
	vec3 color = min((ambient + (1.0f - shadow)*diffuse) + (1.0f - shadow)*specular, 1.0f);

	
	//fog
	float fogFactor = computeFog();
	vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
	
	vec4 ColorCalc = vec4(color,1.0f);
	vec4 fColorMix = mix(fogColor, ColorCalc, fogFactor);

	fColorMix.a = transp;
    fColor = fColorMix;
}

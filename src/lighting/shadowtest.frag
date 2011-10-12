#version 120
//in vec3 norm;

void main(void){
	//gl_FragColor = vec4(norm,1.0);
	//gl_FragColor = vec4(0.0,1.0,0.5,1.0);
	gl_FragColor = gl_Color;
	gl_FragDepth = gl_FragCoord.z;
}
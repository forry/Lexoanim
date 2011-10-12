//varying out vec3 norm;

void main(void)
{
	//gl_Position = ftransform();
	//norm = vec3((gl_ModelViewMatrixInverseTranspose * vec4(gl_Normal,0.0)).xyz);
	gl_Position = gl_ModelViewMatrix * gl_Vertex;
}
/**
 * Shadow volume shader for use with ShadowVolume1 class.
 * Produces volume from triangle.
 */
 
  /* Modes from SVGG
  enum Modes{
      CPU_RAW              = 1, // CPU extrude shadow volume from each triangle
      CPU_SILHOUETTE       = 2, // CPU computes silhouette and then extrude shadow volume
      GPU_RAW              = 3, // GPU extrude shadow volume from each triangle using geometry shader
      GPU_SILHOUETTE       = 4, // GPU computes silhouette and then extrude shadow volume using adjacency information
      CPU_FIND_GPU_EXTRUDE = 5  // CPU finds a silhouette but GPU extrude shadow volume using geometry shader
   };

   enum Methods{
      ZPASS = 1,
      ZFAIL = 2
   };*/

#version 120
#extension GL_EXT_geometry_shader4 : enable

//in vec3 norm[]; //vertex normals - from vertex shader
uniform vec4 lightpos; //light position in world space - from cullVisitor - well it's odd but it's not in world space
uniform int mode;
uniform int just_caps;
 
void main() {
	
	vec4 light;// = gl_ModelViewMatrix * vec4(0.0,-0.0,.0,1.0);
	light =  gl_ModelViewMatrix * lightpos; //why do I have to use modview matrix, when it multiplied in getLightPositionalState()?
	//light =  lightpos;
	float epsilon = 0.1; //tolerance when the light is very near some vetex of occluder
	
	/* detect facing */
	vec4 edg1 = gl_PositionIn[1] - gl_PositionIn[0];
	vec4 edg2 = gl_PositionIn[2] - gl_PositionIn[0];
	vec3 tmp_norm = cross(edg1.xyz,edg2.xyz);
	
	
	/* SHADOW VOLUME triangle only, sides only (no caps)*/
	vec4 col = vec4(0.0,0.5,1.0,1.0); //color for debuging purposes
	vec4 v0 = gl_ProjectionMatrix * gl_PositionIn[0];
	vec4 v1 = gl_ProjectionMatrix * gl_PositionIn[1];
	vec4 v2 = gl_ProjectionMatrix * gl_PositionIn[2];
	
	vec4 v0inf =vec4(gl_PositionIn[0].xyz - light.xyz,0.0);// gl_ModelViewMatrix * vec4(0.0,0.0,0.0,1.0);
	vec4 v1inf =vec4(gl_PositionIn[1].xyz - light.xyz,0.0);// gl_ModelViewMatrix * vec4(1.0,1.0,1.0,0.0);
	vec4 v2inf =vec4(gl_PositionIn[2].xyz - light.xyz,0.0);
	
	
	/* when the light is too close to vertex, then we just project along the -normal  */
	//if(length(v0inf) <= epsilon) v0inf = vec4(-norm[0],0.0);
	//if(length(v1inf) <= epsilon) v1inf = vec4(-norm[1],0.0);
	//if(length(v2inf) <= epsilon) v2inf = vec4(-norm[2],0.0);
	
	/* detect facing */
	//vec3 ref = light.xyz - gl_PositionIn[0].xyz;
	//vec3 volume_side_normal = cross(gl_PositionIn[1].xyz - gl_PositionIn[0].xyz, v0inf.xyz - gl_PositionIn[0].xyz);
	if(dot(tmp_norm,light.xyz - gl_PositionIn[0].xyz)<0){ //if not light facing triangle we must change winding
		//return;
		/* swap points thus changing winding order */
		col = vec4(0.5,0.0,1.0,1.0);
		vec4 tmp = v0;
		v0 = v1;
		v1 = tmp;
		
		tmp = v0inf;
		v0inf = v1inf;
		v1inf = tmp;
	}
	
	v0inf = gl_ProjectionMatrix * v0inf;
	v1inf = gl_ProjectionMatrix * v1inf;
	v2inf = gl_ProjectionMatrix * v2inf;
	
	/*gl_FrontColor = col;
	gl_Position = v0;
	EmitVertex();
	gl_FrontColor = vec4(1.0,0.0,0.0,1.0);
	gl_Position = v1;
	EmitVertex();*/
	
	/* from lines */
	
	/*
	gl_FrontColor = col;
	gl_Position = v1;
	EmitVertex();
	gl_FrontColor = col;
	gl_Position = v1inf;
	EmitVertex();	
	gl_FrontColor = col;
	gl_Position = v0;
	EmitVertex();
	gl_FrontColor = col;
	gl_Position = v0inf;
	EmitVertex();
	*/
	
	if(just_caps == 0 ){
		/* TRIANGLE STRIP */
		//1st side
		gl_FrontColor = col;
		gl_Position = v0;
		EmitVertex();
		gl_FrontColor = col;
		gl_Position = v0inf;
		EmitVertex();
		gl_FrontColor = col;
		gl_Position = v1;
		EmitVertex();
		gl_FrontColor = col;
		gl_Position = v1inf;
		EmitVertex();
		
		//2nd side
		gl_FrontColor = col;
		gl_Position = v2;
		EmitVertex();
		gl_FrontColor = col;
		gl_Position = v2inf;
		EmitVertex();
		
		//3rd side
		gl_FrontColor = col;
		gl_Position = v0;
		EmitVertex();
		gl_FrontColor = col;
		gl_Position = v0inf;
		EmitVertex();
		
		EndPrimitive();
	}
	
	else if(just_caps == 1){ //ZFAIL
		/* light cap */
		gl_FrontColor = col;
		gl_Position = v0;
		EmitVertex();		
		gl_FrontColor = col;
		gl_Position = v1;
		EmitVertex();
		gl_FrontColor = col;
		gl_Position = v2;
		EmitVertex();
		
		EndPrimitive();
		
		/* dark cap */
		gl_FrontColor = col;
		gl_Position = v0inf;
		EmitVertex();		
		gl_FrontColor = col;
		gl_Position = v2inf;
		EmitVertex();
		gl_FrontColor = col;
		gl_Position = v1inf;
		EmitVertex();
		
		EndPrimitive();
	}

}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _SHADER_UTILS
#define _SHADER_UTILS


char *textFileRead(char *fn) {


	FILE *fp;
	char *content = NULL;

	int count=0;

	if (fn != NULL) {
		fp = fopen(fn,"rt");

		if (fp != NULL) {
      
      fseek(fp, 0, SEEK_END);
      count = ftell(fp);
      rewind(fp);

			if (count > 0) {
				content = (char *)malloc(sizeof(char) * (count+1));
				count = fread(content,sizeof(char),count,fp);
				content[count] = '\0';
			}
			fclose(fp);
		}
	}
	return content;
}

int textFileWrite(char *fn, char *s) {

	FILE *fp;
	int status = 0;

	if (fn != NULL) {
		fp = fopen(fn,"w");

		if (fp != NULL) {
			
			if (fwrite(s,sizeof(char),strlen(s),fp) == strlen(s))
				status = 1;
			fclose(fp);
		}
	}
	return(status);
}

#ifdef OSG_SHADER

osg::Program* createProgram(char *vertfile, char *geomfile, char *fragfile){

   const char *code_vert = NULL;
   const char *code_frag = NULL;
   const char *code_geom = NULL;

   bool is_vert = vertfile!=NULL && strcmp(vertfile,"");
   bool is_frag = fragfile!=NULL && strcmp(fragfile,"");
   bool is_geom = geomfile!=NULL && strcmp(geomfile,"");

   osg::Program* pgm = new osg::Program;

   if(is_vert){
      code_vert = textFileRead(vertfile);
      pgm->addShader( new osg::Shader( osg::Shader::VERTEX,   code_vert ) );      
   }

   if(is_frag){
      code_frag = textFileRead(fragfile);
      pgm->addShader( new osg::Shader( osg::Shader::FRAGMENT, code_frag ) );
   }

   if(is_geom){
      code_geom = textFileRead(geomfile);
      pgm->addShader( new osg::Shader( osg::Shader::GEOMETRY, code_geom ) );

   }

   return pgm;
}

#else // not def OSG_SHADER

//Got this from http://www.lighthouse3d.com/opengl/glsl/index.php?oglinfo
// it prints out shader info (debugging!)

void printShaderInfoLog(GLuint obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;
    glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("printShaderInfoLog: %s\n",infoLog);
        free(infoLog);
	}else{
		printf("Shader Info Log: OK\n");
	}
}

void printProgramInfoLog(GLuint obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;
	glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("printProgramInfoLog: \n%s\n",infoLog);
        free(infoLog);
    }else{
		printf("Program Info Log: OK\n");
	}
}

void setProgram(GLuint *vert, GLuint *frag, GLuint *geom, GLuint *prog, char *vertfile, char *fragfile, char *geomfile){
   const char *code_vert = NULL;
   const char *code_frag = NULL;
   const char *code_geom = NULL;

   bool is_vert = vert!=NULL && vertfile!=NULL && strcmp(vertfile,"");
   bool is_frag = frag!=NULL && fragfile!=NULL && strcmp(fragfile,"");
   bool is_geom = geom!=NULL && geomfile!=NULL && strcmp(geomfile,"");
   if(is_vert){
      code_vert = textFileRead(vertfile);
      *vert = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(*vert,1, &code_vert, NULL);
      glCompileShader(*vert);
   }

   if(is_frag){
      code_frag = textFileRead(fragfile);
      *frag = glCreateShader(GL_FRAGMENT_SHADER);
      glShaderSource(*frag,1, &code_frag, NULL);
      glCompileShader(*frag);
   }

   if(is_geom){
      code_geom = textFileRead(geomfile);
      *geom = glCreateShader(GL_GEOMETRY_SHADER_EXT);
      glShaderSource(*geom,1, &code_geom, NULL);
      glCompileShader(*geom);
   }

   if(prog!=NULL){
      *prog = glCreateProgram();
      if(is_vert) glAttachShader(*prog,*vert);
      if(is_frag) glAttachShader(*prog,*frag);
      if(is_geom) glAttachShader(*prog,*geom);
      //glLinkProgram(*prog);
   }
   //free(code`_vert);free(code_frag);

}

#endif //OSG_SHADER

#endif //_SHADER_UTILS
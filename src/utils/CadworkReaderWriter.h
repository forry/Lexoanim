/**
 * @file
 * CadworkReaderWriter class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef CADWORK_READER_WRITER_H
#define CADWORK_READER_WRITER_H

#include <osgDB/ReaderWriter>


/**
 * The class for reading/writing ivx and ivl files.
 *
 * ivx file extension was introduced by Cadwork company
 * to distinguish Inventor Ascii (ivx) and Inventor Binary (iv)
 * files by file extension. Originally, Inventor uses
 * the same extension (iv) to both formats.
 *
 * ivl file extension is used by Cadwork software for
 * Inventor models (ascii and binary) that contains the scene
 * with light setup inside. One of applications of such models
 * is photorealistic scene rendering.
 *
 * This plugin helps OSG to handle Cadwork file extension
 * while the plugin just forwards all the requests
 * to the original OSG Inventor plugin.
 */
class CadworkReaderWriter : public osgDB::ReaderWriter
{
public:

   // Constructor and destructor
   CadworkReaderWriter();
   virtual ~CadworkReaderWriter();

   virtual const char* className() const { return "Cadwork Reader/Writer"; }

   static void createAliases();

   virtual osgDB::ReaderWriter::ReadResult readObject(
         const std::string &file, const Options *opt ) const;
   virtual osgDB::ReaderWriter::ReadResult readNode(
         const std::string &file, const Options *opt ) const;
   virtual osgDB::ReaderWriter::ReadResult readObject(
         std::istream &fin, const Options* options ) const;
   virtual osgDB::ReaderWriter::ReadResult readNode(
         std::istream &fin, const Options* options ) const;

};


#endif /* CADWORK_READER_WRITER_H */

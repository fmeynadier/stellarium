#ifndef HEIGHTMAP_HPP
#define HEIGHTMAP_HPP

#include "OBJ.hpp"

//! This represents a heightmap for viewer-ground collision
class Heightmap
{

public:

   //! Construct a heightmap from a loaded OBJ mesh.
   //! The mesh is stored as reference and used for calculations.
   //! @param obj Mesh for building the heightmap.
   Heightmap(const OBJ& obj);
   virtual ~Heightmap();

   //! Get z Value at (x,y) coordinates.
   //! In case of ambiguities always returns the maximum height.
   //! @param x x-value
   //! @param y y-value
   //! @return z-Value at position given by x and y
   float getHeight(float x, float y);

private:

   static const int GRID_LENGTH = 1; // # of grid spaces is GRID_LENGTH^2

   typedef std::vector<const OBJ::Face*> FaceVector;
   
   struct GridSpace {
      FaceVector faces;
      float getHeight(const OBJ& obj, float x, float y);
      
      static float face_height_at(const OBJ& obj, const OBJ::Face* face, float x, float y);
   };

   const OBJ& obj;
   GridSpace* grid;
   float xMin, yMin;
   float xMax, yMax;

   void initGrid();
   GridSpace* getSpace(float x, float y);
   bool face_in_area (const OBJ::Face* face, float xmin, float ymin, float xmax, float ymax);

};

#endif // HEIGHTMAP_HPP

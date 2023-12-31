# Voxel Engine utilizing lattice chunks

## Graphics FAQ

 - What is a lattice chunk?

 typically voxels are rendered in one of 2 ways, meshes or raymarching.

 ===

 Techniques generating meshes take precious cpu resources and are a relatively slow operations as well.
 Naive mesh generation like in Minecraft cull interior faces, but are still not extremely performant
 once on the gpu. You can save gpu resources by greedy meshing the vertices in the chunks and also reduce
 data sent to the graphics card making the time in between frames shorter, which is always good. You can
 improve generation speed by compressing voxel data into bits representing greedable voxels and not greedable
 voxels, which increases cpu cache hits and improves cpu operations significantly as well. This is very fast,
 incredibly fast, but there are still potential operations on the table.

 ===

 Raymarching uses math and the gpu heavily to compute the visual data of voxels,
 this is a very gpu heavy approach, which is good because it is inherently parrallelized,
 but raymarching itself is still an expensive operation because as rays get closer to the edges of a volume
 the distance between the next ray decreases and forces more iterations.

 ===

 The final theoretically most efficient technique for generic voxels: lattice chunks
 
 We essentially pre-compute ALL POSSIBLE configurations of a chunk.
 For each axis we create one large face fitting a desired size.
 Using multi-drawing we can make thousands without sending duplicate data if required.
 We can also just make one massive global lattice reducing the required vertex count lower instead
 of also chunking our vertex data on the graphics card.
 Essentially by doing this we move as much computation to the GPU as possible.
 Using a fragment shader we can create a texture to be mapped to the entire face super quickly.
 This in theory makes chunk updates insanely fast to render because the only real limiting factor
 is the time it takes to transfer chunk data instead of waiting for the data to be processed into
 a new mesh

 Does this work for all voxels? No, for more complicated geometry this approach doesn't work at all
 because the only reason this works is because we assume the voxel's faces are always in the same places,
 if you want anything more complicated than a cube with a bump map this technique won't work.
 However this allows us to render incredible distances of regular voxel terrain with less performance impact.

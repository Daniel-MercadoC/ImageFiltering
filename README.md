# ImageFiltering
Tests and exploration for basic image filtering made for class at Querétaro's Autonomous University (UAQ by it's signals in spanish) for the Master's course in Computational Systems.

The files were made for better understanding how image filtering works, and C was chosen as an alternative to Python (which is recommended in class) for this project to better compare the differences between these two languages.

## Dependencies
- [stb_image](https://github.com/nothings/stb.git)

## Usage
The file included inside "InitialTest" was purely written for understanding from a programmer's perspective, therefore it's necessary to edit and compile it directly to be used. It provides structures for kernels to be defined by the programmer for both Gaussian blur and a basic Sharpen filter, and any file's path has to be entered inside.

Meanwhile, the file included inside "Optimizations" was made as an optimization excercise, which also provides user interaction via the terminal. It also needs compilation, but can be used afterwards through a terminal for different files. However, this only provides a Gaussian blur.

#### Steps
1. Download dependencies
2. Enter the respective "filtros.c" file
3. Edit the <code>#include</code> path for both <code>stb_image.h</code> and <code>stb_image_write.h</code> depending on where those files can be found
4. If using the file in "InitialTest" edit the strings in <code>LoadRGBImage()</code> and <code>SaveImage()</code> for input and output file respectively
5. Compile with the following line:
   <code>gcc -o filtros filtros.c -lm -O3 -march=native -ffast-math -mfma -mavx2</code>
   (only up to -lm is strictly necessary, but the rest are optimization flags for better performance on large images)
6. Run the newly created file in a terminal. If the optimized version is used, running it will yield a message on how to provide the input and output paths.
   

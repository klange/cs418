# Shiny Teapots and Whatnot

# Invocation

	make
	./mp3 [-h N.N] [-s N.N] [-d <file>] [-e <file>] [<file>]

	Or, run ./mp3-rgba <similarly> for the RGBA-enabled mode (only works on newer cards under Linux with a compositor!)
	Note that make will undoubtedly fail on non-X11 systems.

	-h
		Set camera height offset (default is 1.0)
	-s
		Set scene scale (default is 1.0)
	-d
		Use <file> for diffuse texture. Must be a raw RGBA bitmap (use `convert [file] [output].rgba` from the ImageMagick toolkit)
	-e
		Use <file> for environment map. Must be a raw RGBA bitmap
	<file>
		Load the given Wavefront obj model (default is 'teapot.obj')

# Keyboard Controls

	ESC
		Quit the application.
	w
		Raise the camera's orbit
	s
		Lower the camera's orbit
	p
		Pause / unpause object rotation

# Mouse

	Mouse movement controls the light position

# Try it:

	./mp3
		To run with the standard teapot
	./mp3 -h 2.0 -s 0.5 bunny.obj
		To run with the Stanford bunny

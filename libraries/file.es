with-dynlibs mod_file {
	library file (init libraries)

	fn-%file_open = $&file_open
	fn-%file_name = $&file_name
	fn-%file_fd = $&file_fd
	fn-%file_read = $&file_read
	fn-%file_write = $&file_write
	fn-%file_seek = $&file_seek
}


template <typename LibraryClass>
void Library::initialise_static()
{
	if (!s_main)
	{
		s_main = new LibraryClass();
		s_main->initialise();
	}
	if (!s_current)
	{
		s_current = s_main;
	}
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (!s_libraryLoadSetup)
	{
		s_libraryLoadSetup = new LibraryLoadSetup();
	}
#endif
}

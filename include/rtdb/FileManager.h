#ifndef _RTDB_FILEMANAGER_H_
#define _RTDB_FILEMANAGER_H_

#include <rtdb/IFileLoader.h>
#include <map>

namespace rtdb {

class FileManager
{
public:
	static FileManager* instance();

	// Add loader
	void addFileLoader( IFileLoader* loader );

	// Called by added Loader to register supported extensions, i.e. extension = ".obj"
	void registerLoaderToExtension( IFileLoader* loader, const vr::String& extension );

	// Main loading method
	bool loadGeometry( const vr::String& filename );

	// Filters to use in open file dialog
	vr::String getFileFilters();

private:
	// Singleton
	FileManager();

	typedef std::map< vr::String, vr::ref_ptr<IFileLoader> > LoaderContainer;
	LoaderContainer _loaders;
};

} // namespace rtdb

#endif // _RTDB_FILELOADER_H_

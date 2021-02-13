#include <rtdb/FileManager.h>

using namespace rtdb;

FileManager* FileManager::instance()
{
	static FileManager s_fileManagerInstance;
	return &s_fileManagerInstance;
}

void FileManager::addFileLoader( IFileLoader* loader )
{
	loader->registerSupportedExtensions();
}

void FileManager::registerLoaderToExtension( IFileLoader* loader, const vr::String& extension )
{
	// Avoid duplicates
	LoaderContainer::iterator itr = _loaders.find( extension );
	if( itr != _loaders.end() )
		return;

	_loaders.insert( std::make_pair( extension, loader ) );
}

bool FileManager::loadGeometry( const vr::String& filename )
{
	vr::String ext = rtdb::getExtension( filename );
	LoaderContainer::iterator itr = _loaders.find( ext );
	if( itr == _loaders.end() )
	{
		printf( "Could not find loader for extension \'%s\'.", ext.data() );
		return false;
	}

	return itr->second->loadGeometry( filename );
}

vr::String FileManager::getFileFilters()
{
	vr::String filters( "Geometry files (" );

	for( LoaderContainer::iterator itr = _loaders.begin(); itr != _loaders.end(); ++itr )
	{
		filters += "*" + itr->first + "; ";

		//filters.appendFormatted( "*%s; ", itr->first.data() );
	}

	//filters.appendFormatted( ");;All files (*.*)" );

	filters += ");;All files (*.*)";

	return filters;
}

// Private

FileManager::FileManager()
{
	// empty
}

#include <iostream>
#include <iterator>
#include <queue>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <future>

#include "Picosha2.hpp"

using namespace std;

struct DuplicateFileFinder {
   mutex myMutex;
   condition_variable workReadyCV;
   queue<filesystem::path> readyQueue;

   // (hash, file paths)
   unordered_map<string, vector<string> > fileHashes;

   atomic<bool> quit { false };

   // Build the above variables / data structures in their default state
   DuplicateFileFinder() = default;
   DuplicateFileFinder(const DuplicateFileFinder&) = delete;
   DuplicateFileFinder& operator=(const DuplicateFileFinder&) = delete;
};

// This function is used to hash a given file
string fileHash(const filesystem::path& path) {

   // open the file located at the given path
   ifstream file(path, ios::binary);

   // fills the vector with the recommended size to perform the sha256 algorithm
   vector<unsigned char> hash(picosha2::k_digest_size);

   // Hashes the file and store the result into the hash vector in bytes
   picosha2::hash256( file, begin(hash), end(hash) );

   // converts the bytes to string
   return picosha2::bytes_to_hex_string( hash.begin(), hash.end() );
}

// Assign work to every thread
// Threads take work from the readyQueue and places the result into the map(fileHashes)
void assignTask(DuplicateFileFinder& finder) {
   while(true) {
      filesystem::path fileToHash;

      // Note this lock is only valid inside this scope 
      {
         unique_lock lock(finder.myMutex);
         
         if(finder.quit && finder.readyQueue.empty()) 
            break;

         else if( finder.readyQueue.empty() ) {
            cout << "Thread(" << this_thread::get_id() << "): Waiting for files to scan" << endl;

            // Waiting for work
            finder.workReadyCV.wait(lock, [&finder] { return !finder.readyQueue.empty();} );
         }

         // Assign work to fileToHash
         fileToHash = finder.readyQueue.front();

         cout << "Thread(" << this_thread::get_id() << "): Hashing file @ \n\t" << fileToHash << endl;

         // Removes the completed task
         finder.readyQueue.pop();
      }

      // Runs asynchronously, therefore any thread can operate on the hash
      string hash = fileHash(fileToHash);

      {
         unique_lock lock(finder.myMutex);
  
         if(finder.fileHashes.find(hash) != finder.fileHashes.end() ) {
            // Add duplicate file to the map
            finder.fileHashes.at(hash).push_back( fileToHash.string() );
         }
         else {
            vector<string> filePaths{ fileToHash.string() };
            finder.fileHashes.insert({ hash, filePaths });
         }
      }
   }
}

void printDuplicates(const DuplicateFileFinder& finder) {
   for_each( begin(finder.fileHashes), end(finder.fileHashes), [](const auto& myMap) {
      const vector<string>& filePaths = myMap.second;

      if(filePaths.size() > 1) {
         cout << "Duplicates found: " << endl;

         for_each(begin(filePaths), end(filePaths), [](const string& filePath) {
            cout << "\t" << filePath << endl;
         });
      }
   });
}

int main(int argc, char* argv[]) {

   if(argc <= 1) {
      cerr << "Please provide a path to search " << endl;
      return 1;
   }

   filesystem::directory_entry startingDir(argv[1]);

   if( !startingDir.is_directory() ) {
      cerr << "Please specify a directory to start from" << endl;
      return 2;
   }

   DuplicateFileFinder finder;

   queue<filesystem::directory_entry> directories;
   directories.push(startingDir);

   vector<thread> threads;

   // Number of threads your system supports
   unsigned int numberOfThreads = thread::hardware_concurrency();

   // Assign for to all the newly created threads
   for(unsigned int i = 0; i < numberOfThreads; i++) {
      thread myThread( &assignTask, std::ref(finder) );
      threads.push_back( std::move(myThread) );
   }

   while( !directories.empty() ) {
      filesystem::directory_entry mainDirectory = directories.front();
      directories.pop();

      for(auto& directory : filesystem::directory_iterator(mainDirectory) ) {
         if( directory.is_regular_file() ) {
            {
               unique_lock lock(finder.myMutex);
               finder.readyQueue.push( directory.path() );

               cout << "Add File to Ready Queue: \n\t" << directory.path().string() << endl;
            }

            finder.workReadyCV.notify_all();
         }

         // Allows recursive searching
         else if( directory.is_directory() )
            directories.push(directory);

         else
            continue;
      }
   }

   // Let the program know that i'm ready to terminate 
   finder.quit.store(true);

   // Wait for all the threads to finish their execution
   for_each( begin(threads), end(threads), [](thread& currentThread) {currentThread.join();} );

   cout << "Hashed " << finder.fileHashes.size() << " unique files " << endl;

   printDuplicates(finder);

   return 0;
}
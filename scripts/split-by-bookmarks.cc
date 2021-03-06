#include <stack>
#include <string>
#include <iostream>
#include <vector>

#include <pstreams/pstream.h>

struct Bookmark {
  std::string title{};

  int level{};
  int page_number{};
  int number{};
};

std::string extract_value(const std::string &line, const std::string &key) {
  std::size_t pos = line.find(key);

  if (pos != std::string::npos) {
    return line.substr(pos + key.size());
  } else {
    return "";
  }
}

int extract_int_value(const std::string &line, const std::string &key) {
  return std::stoi(extract_value(line, key));
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "file name required" << std::endl;
    std::exit(1);
  }
  std::string line{};
  std::stack <Bookmark> s{};

  Bookmark B;
  bool document_begin = true;
  int counter{};
  int parent_counter{};
  int all_pages{};

  // create input stream
  redi::ipstream in({"/usr/bin/pdftk", (std::string)argv[1], "dump_data_utf8"});

  while (std::getline(in, line)) {
    switch (counter) {
    case 0 :
      if (line.find("NumberOfPages: ") != std::string::npos) {
        all_pages = extract_int_value(line, "NumberOfPages: ");
      } else if (line.find("BookmarkBegin") != std::string::npos) {
        counter = 1; // begin of bookmark block
      }
      break;
    case 1 :
      B.title = extract_value(line, "BookmarkTitle: ");
      counter++; // proceed to next value
      break;
    case 2 :
      B.level = extract_int_value(line, "BookmarkLevel: ");
      counter++; // proceed to next value
      break;
    case 3 :
      B.page_number = extract_int_value(line, "BookmarkPageNumber: ");
      counter = 0; // block completed

      if (B.level == 1) {
	// first level bookmarks -> chapters
	parent_counter++;
      } else if (B.level == 2) {
	// second level bookmarks -> sections
	if (document_begin) {
	  // first bookmark
	  if (B.page_number > 1) {
	    // preamble (document before first bookmark)
	    std::cout << 1 << " " << B.page_number-1 << std::endl;
	  }
	  document_begin = false;
	} else if (!s.empty()) {
	  // previous bookmark found
	  Bookmark B_prev = s.top();
	  s.pop();

	  // if multiple bookmarks on same page, name after first bookmark
	  if (B.page_number > B_prev.page_number) {
	    // continue to previous page of next bookmark
	    std::cout << B_prev.page_number << " " << B.page_number-1 << " "
		      << B_prev.title << std::endl;
	  }
	}
	s.push(B);
      }
      break;
    }
  }

  // process last stack entry
  if (!s.empty()) {
    Bookmark B_last = s.top();
    s.pop();

    std::cout << B_last.page_number << " " << all_pages << " "
	      << B_last.title << std::endl;
  }
  return 0;
}

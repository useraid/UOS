// eReader

// Variables
int selectedFileIndex = 0;
int numberOfFiles = 0;

// Array of text files, adjust the size and content as needed
const String fileContents[] = {
  "Text file 1 content... wytjdfctyfvkytfvtyf tkyftyf yt fty fytkfty fytf tytkytgvyulgvuygyugb uy guygyug uy ghyuguygyugug ug uyg uhbuhbhjb hjbjh bjhb hjbhj xdfgfd dfs gfd gsd sd d gsd ds gsdg sdg ds gdsg ",
  "Text file 2 content...wytjdfctyfvkytfvtyf tkyftyf yt fty fytkfty fytf tytkytgvyulgvuygyugb uy guygyug uy ghyuguygyugug ug uyg uhbuhbhjb hjbjh bjhb hjbhjsfdg fdsg fdsg fdg ds sd gsdg sdsg sd dfsg sd d d g",
  "Text file 3 content...wytjdfctyfvkytfvtyf tkyftyf yt fty fytkfty fytf tytkytgvyulgvuygyugb uy guygyug uy ghyuguygyugug ug uyg uhbuhbhjb hjbjh bjhb hjbhj sfgfd gfdgsdfsg sdfgsfdgfd gf sfd fd gfd sdfgsdf f"
};

// Array of text files, adjust the size and content as needed
const String fileHeadings[] = {
  "Heading 1",
  "Heading 2",
  "Heading 3"
};

// Number of text files
const int numFiles = sizeof(fileHeadings) / sizeof(fileHeadings[0]);

int scrollOffset = 0;

// Reader state
enum MenuState {
  FILE_SELECTION,
  FILE_VIEWING
};
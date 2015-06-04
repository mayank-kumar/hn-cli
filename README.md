# hn-cli
## A command line Hacker News client for Windows

- Get the titles of top HN stories in your console
- **Skip stories** - the next time you run the tool, you'll see only unskipped stories
- Launch any story in the default browser

### Usage
The interface is ~~still somewhat cruddy~~ getting better. Here are the basics:
- There are 10 articles on a page
- The active article has an asterisk to its left
- Press the arrow keys to navigate without skipping:
  - top and down to move between stories
  - left and right to move between pages
- Press 'enter' to launch the active article in the system default browser
- Press 'n' to go to the next story and mark the current one skipped
- Press 'p' to go to the previous story and mark the current one skipped
- Press 'page down' to go to the next page and mark all stories on the current page skipped
- Press 'page up' to go to the previous page and mark all stories on the current page skipped
- Press 'q' to quit

### To build
You'll need:
- a PC with Windows (duh!)
- Visual C++ (2013 or later)
- [rapidjson](https://github.com/miloyip/rapidjson) (put the header files anywhere in your project's INCLUDE path)

And that's it.

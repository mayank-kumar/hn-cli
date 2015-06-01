# hn-cli
## A command line Hacker News client for Windows

- Get the titles of top HN stories in your console
- **Skip stories** - the next time you run the tool, you'll see only unskipped stories
- Launch any story in the default browser

### Usage
The interface is still somewhat cruddy, but here are the basics:
- There are 10 articles on a page
- The active article has an asterisk to its left
- Press 'n' to go to the next story
- Press 'tab' to go to the next page
- Press 'p' to go to the previous story
- Press 'backspace' to go to the previous page
- Press 's' to go to the next page **and skip the stories on the current page**
- Press 'enter' to launch the article in the system default browser
- Press 'q' to quit

### To build
You'll need:
- a PC with Windows (duh!)
- Visual C++ (2013 or later)
- [rapidjson](https://github.com/miloyip/rapidjson) (put the header files anywhere in your project's INCLUDE path)

And that's it.

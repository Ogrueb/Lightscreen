# Naming dialog's character filter and folder suggestions: re-checks (M04, bugs 40 and 41)

Step 1 ("Go to folder" with a space in the path) passed in the 2026-10-23 pass. Use a PR #58 build or later.

1. **Date format (bug 40).** In Options, set Naming to Date and press the button next to it to open the Naming dialog. In its date format box, typing `?`, `:` or `*` does nothing; letters, digits, `-` and `.` still type. Pasting text with `:` in it is refused too.
2. **Folder suggestions (bug 41).** In Options, clear the Directory box and type `C:\` and then `U`: a list drops down with `C:\Users`. Go on typing a path; after each `\` the subfolders of that folder are listed, with no `.` or `..`. Lower case works too (`c:\u`).

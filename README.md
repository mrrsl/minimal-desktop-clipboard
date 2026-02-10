Minimal demo demonstrating how to retrieve clipboard data on both Windows and Linux environments using X11.
## Usage
```c++

using ClipLib::GetClipboard;
using ClipLib::SetClipboard;

int main() {

    cout << "Clipboard text: " << ClipLib::GetClipboard() << endl;

    auto my_text =  "Ship it";
    ClipLib::SetClipboard(my_text);

    cout << "Clip it " << ClipLib::GetClipboard() << endl;

}
```
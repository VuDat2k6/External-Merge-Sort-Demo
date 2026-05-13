# External Merge Sort - Minh họa và Cài đặt

Dự án này bao gồm việc triển khai và minh họa trực quan thuật toán **External Merge Sort**. Dự án bao gồm hai phần chính: một ứng dụng Desktop backend C++ có giao diện (GUI) bằng wxWidgets để xử lý thực tế, và một ứng dụng Web tương tác để mô phỏng và học tập.

## 📌 Tổng quan dự án

External Merge Sort là một thuật toán sắp xếp được thiết kế đặc biệt để xử lý các tập dữ liệu khổng lồ không thể chứa vừa trong bộ nhớ chính (RAM) của máy tính. Dự án này phục vụ hai mục đích:
1. **Minh họa học tập:** Một ứng dụng web tương tác trực quan theo từng bước để hiểu cách hoạt động của thuật toán K-way merge sort.
2. **Cài đặt thực tế:** Một bản cài đặt bằng C++ mạnh mẽ có khả năng sắp xếp các tệp nhị phân có dung lượng lớn.

## ✨ Tính năng nổi bật

### 🌐 Web Demo Tương tác
- **Minh họa từng bước:** Theo dõi thuật toán K-way merge hoạt động qua từng giai đoạn (Chia Chunk, Sắp xếp Chunk, Gộp K-way).
- **Tùy chỉnh tham số:** Dễ dàng điều chỉnh số lượng phần tử, kích thước chunk và giá trị `K` (số đường merge).
- **Giao diện hiện đại:** Giao diện Dark theme, thiết kế glassmorphism với các thẻ dữ liệu được mã hóa màu sắc rõ ràng.
- **Không cần cài đặt:** Không yêu cầu server. Toàn bộ quá trình mô phỏng chạy trực tiếp ngay trên trình duyệt web của bạn.

### 🖥️ Ứng dụng Desktop (C++ / wxWidgets)
- **Sắp xếp hiệu suất cao:** Cài đặt thuật toán external sort cho kiểu số thực độ chính xác kép (`Float64`) sử dụng tệp nhị phân.
- **Xử lý ngoài bộ nhớ (Out-of-Core):** Sắp xếp dữ liệu tuần tự sử dụng thao tác đọc/ghi ổ cứng, tuân thủ nghiêm ngặt các giới hạn về bộ nhớ.
- **Giao diện điều khiển (GUI):** Chọn tệp đầu vào, cấu hình giới hạn kích thước bộ nhớ, và theo dõi tiến trình qua giao diện Desktop.

## 🚀 Hướng dẫn cài đặt và chạy ứng dụng

Sau khi bạn pull (hoặc clone) dự án này từ GitHub về máy, hãy làm theo các hướng dẫn dưới đây để chạy ứng dụng.

### 1. Chạy Web Visualizer (Trình minh họa Web)
Web demo là một ứng dụng độc lập và rất dễ chạy:
1. Mở thư mục dự án vừa tải về.
2. Click đúp chuột vào tệp `index.html` để mở nó bằng bất kỳ trình duyệt web hiện đại nào (Chrome, Edge, Firefox, Safari,...).
3. Sử dụng giao diện trên web để tải lên file nhị phân (hoặc sinh dữ liệu ngẫu nhiên) và nhấn "Bắt đầu sắp xếp".

### 2. Biên dịch và chạy ứng dụng Desktop (C++)
Để chạy được phần mềm Desktop thực tế, máy tính của bạn cần được cài đặt môi trường C++.

**Yêu cầu hệ thống:**
- Một trình biên dịch C++ (GCC, MinGW, Clang, hoặc MSVC/Visual Studio).
- Thư viện [wxWidgets](https://www.wxwidgets.org/) đã được cài đặt và cấu hình đường dẫn (Path) trên máy.

**Cách build trên Visual Studio (Windows):**
1. Mở Visual Studio và tạo một project C++ mới dạng Desktop Application. (Hoặc nếu trong thư mục cha có file `.sln` thì mở file đó).
2. Thêm tất cả các file mã nguồn (như `App.cpp`, `MainFrame.cpp`, `DemoController.cpp`, `ExternalSorter.cpp`...) vào project.
3. Cấu hình Include và Library Directories trỏ đến thư mục wxWidgets của bạn.
4. Build (nhấn Ctrl + Shift + B) và chạy chương trình (nhấn F5).

**Cách build bằng Command Line (với g++ / MinGW):**
```bash
# Ví dụ cơ bản để compile với wxWidgets (yêu cầu wx-config đã có trong PATH)
g++ *.cpp `wx-config --cxxflags --libs` -o ExternalMergeSortApp

# Chạy file thực thi
./ExternalMergeSortApp
```

## 🧠 External Merge Sort hoạt động như thế nào?
1. **Giai đoạn Chunking:** Tệp dữ liệu khổng lồ đầu vào được đọc thành các đoạn (chunk) nhỏ hơn sao cho vừa vặn với dung lượng RAM cho phép.
2. **Giai đoạn Sorting:** Mỗi chunk được sắp xếp trực tiếp trong bộ nhớ RAM và sau đó ghi ngược trở lại ổ đĩa cứng dưới dạng một "run" (tệp tạm thời đã được sắp xếp).
3. **Giai đoạn K-way Merge:** Các run đã sắp xếp sẽ được gộp lại với nhau, mỗi lần gộp `K` run. Thuật toán sẽ đọc các phần tử nhỏ nhất từ mỗi run, tìm ra giá trị nhỏ nhất trong số đó và ghi vào tệp kết quả. Quá trình này lặp lại cho đến khi toàn bộ dữ liệu được gộp lại thành một tệp duy nhất đã được sắp xếp hoàn chỉnh.

## 📄 Cấu trúc thư mục
- `index.html` - Bản demo minh họa thuật toán trên Web.
- `SPEC.md` - Thông số kỹ thuật và bản thiết kế UI/UX cho Web demo.
- `*.cpp` & `*.h` - Mã nguồn C++ chứa lõi xử lý External Sorter và giao diện wxWidgets.
- `test.js` & `integration_test.js` - Các tệp test logic cho quá trình minh họa trên Web.
- `test_data.bin` / `test_data_sorted.bin` - Tệp nhị phân mẫu để test chức năng của ứng dụng C++.

---
*Dự án được tạo ra với mục đích giáo dục, giúp tìm hiểu các thuật toán sắp xếp nâng cao và quá trình xử lý dữ liệu kích thước lớn.*

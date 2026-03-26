#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>

using namespace std;

// 学生结构体：存储完整信息
struct Student {
    string id;
    string name;
    string className;
    string subjectName;
    int type; // 0:笔试, 1:机考
};

// 工具函数：获取考试类型文字
string getTypeLabel(int type) {
    return (type == 0) ? "笔试" : "机考";
}

int main() {
    // 1. 读取 CSV 数据
    ifstream inFile("data.csv");
    if (!inFile) {
        cerr << "错误：找不到 data.csv 文件！请确保文件与程序在同一目录。" << endl;
        system("pause");
        return 1;
    }

    map<string, vector<Student>> subjectGroups; // 按科目名存学生
    map<string, int> subjectTypeMap;            // 存科目类型
    map<string, set<string>> subjectStudentsSet; // 用于高效冲突检测
    string line;

    int totalRead = 0;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string sub, typeStr, id, name, cls;

        // 解析格式: 科目,类型,学号,姓名,班级
        if (getline(ss, sub, ',') && getline(ss, typeStr, ',') &&
            getline(ss, id, ',') && getline(ss, name, ',') && getline(ss, cls, ',')) {

            Student s = { id, name, cls, sub, stoi(typeStr) };
            subjectGroups[sub].push_back(s);
            subjectTypeMap[sub] = s.type;
            subjectStudentsSet[sub].insert(id);
            totalRead++;
        }
    }
    inFile.close();
    cout << "成功读取数据：" << totalRead << " 条。" << endl;

    // 2. 冲突检测逻辑：将科目分配到不同的时间段(Slot)
    vector<vector<string>> slots;
    for (auto const& pair : subjectGroups) {
        const string& subName = pair.first;
        bool placed = false;

        for (auto& slot : slots) {
            bool conflict = false;
            for (const string& existingSub : slot) {
                // 检查学号是否有交集
                for (const string& stuId : subjectStudentsSet[subName]) {
                    if (subjectStudentsSet[existingSub].count(stuId)) {
                        conflict = true; break;
                    }
                }
                if (conflict) break;
            }
            if (!conflict) {
                slot.push_back(subName);
                placed = true;
                break;
            }
        }
        if (!placed) slots.push_back({ subName });
    }

    // 3. 分配考场并输出到最终 CSV
    ofstream outFile("final_exam_plan.csv");
    // 写入 UTF-8 BOM，防止 Excel 打开中文乱码
    outFile << (char)0xEF << (char)0xBB << (char)0xBF;
    // 表头：按要求顺序排列
    outFile << "学号,姓名,班级,考试科目,考场编号,考试类型,考试时间" << endl;

    for (size_t i = 0; i < slots.size(); ++i) {
        // 每一场内，笔试(0)和机考(1)分别处理，互不混用
        for (int type = 0; type <= 1; ++type) {
            vector<Student> pool;
            for (const string& subName : slots[i]) {
                if (subjectTypeMap[subName] == type) {
                    for (auto& s : subjectGroups[subName]) pool.push_back(s);
                }
            }

            if (pool.empty()) continue;

            // 设置人数约束
            int maxCap = (type == 0) ? 90 : 70;
            int minCap = (type == 0) ? 40 : 20;

            // 计算所需考场数
            int numRooms = ceil((double)pool.size() / maxCap);
            // 尽量满足最小人数约束（除非总人数就少于minCap）
            while (numRooms > 1 && (int)(pool.size() / numRooms) < minCap) {
                numRooms--;
            }

            // 平均分配学生
            int base = pool.size() / numRooms;
            int extra = pool.size() % numRooms;
            int currentPtr = 0;

            for (int r = 0; r < numRooms; ++r) {
                int rSize = base + (r < extra ? 1 : 0);
                string roomID = getTypeLabel(type) + "-" + to_string(r + 1);

                for (int j = 0; j < rSize; ++j) {
                    const Student& s = pool[currentPtr + j];
                    // 核心输出：学号,姓名,班级,科目,考场,类型,时间
                    outFile << s.id << ","
                        << s.name << ","
                        << s.className << ","
                        << s.subjectName << ","
                        << roomID << ","
                        << getTypeLabel(type) << ","
                        << "第" << i + 1 << "时间段" << endl;
                }
                currentPtr += rSize;
            }
        }
    }

    outFile.close();
    cout << "------------------------------------------" << endl;
    cout << "排考完成！已生成文件：final_exam_plan.csv" << endl;
    cout << "请直接使用 Excel 打开该文件查看。" << endl;
    system("pause"); // 防止窗口一闪而过

    return 0;
}
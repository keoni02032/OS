#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>


using namespace std;

int NOabs(int val)
{
    if (val >= 0) {
        return -val;
    }
}

vector<int> Get(string &s)
{
    vector<int> result;

    int val = 0, i = 0;
    int flag = 0;

    while (i < s.size()) {
        if ((s[i] >= '0' && s[i] <= '9') || (s[i] == '-')) {
            if (s[i] == '-') {
                flag = 1;
            }
            if (s[i] >= '0' && s[i] <= '9') {
                const int d = s[i] - '0';
                val = val * 10 + d;
            }
        }

        if (s[i + 1] == ' ' || i + 1 == s.size()) {
            if (flag == 0) {
                result.push_back(val);
                val = 0;
            } else if (flag == 1) {
                val = NOabs(val);
                result.push_back(val);
                val = 0;
            }
        }

        ++i;
    }

    return result;
}

mutex mtx;

void* mul(vector<int> &a, vector<int> &b, vector<int> &c, int j)
{
    mtx.lock();

        j -= 1;

        for (int i = 0; i < b.size(); ++i) {
            c[i + j] += a[j] * b[i];
        }
    mtx.unlock();
}

int main()
{
    string str, str1;
    string a;
    int n;
    cin >> n;
    vector<int> res;
    vector<int> res1;
    vector<int> result;
    vector<int> result1;

    vector<int> ressss;

    vector<string> vec;
    int step = 0, delta = 0;
    for (int i = 0; i <= n; ++i) {
        getline(cin, a);

        str1 = str;
        str = a;

        vec.push_back(a);

        res = Get(str);
        res1 = Get(str1);

        step += res.size();

        if (i >= 2) {
            step -= 1;
        }

        if ((i == 1) && (n == 1)) {
            cout << vec[i - 1] << " " << endl;
            break;
        }

        for (int j = 0; j < step - result.size() + 1; ++j) {
            result.push_back(0);
        }
        if (i == 2) {

            vector<thread> re;
            re.reserve(result.size());

            for(int j = 0; j < result.size(); ++j) {

                re.emplace_back([&res, &res1, &result, j] { 
                    mul(res, res1, result, j);
                });
            }

            for(int l = 0; l < result.size(); ++l) {
                re[l].join();
            }
        }

        if (i >= 3) {
            
            vector<thread> re;
            re.reserve(result.size());

            res1.clear();

            for (int s = 0; s < result.size(); ++s) {
                res1.push_back(result[s]);
                result[s] = 0;
            }


            for (int j = 0; j < result.size(); ++j) {

                re.emplace_back([&result, &res, &res1, j] {
                    mul(res1, res, result, j);
                });
            }


            for (int l = 0; l < result.size(); ++ l) {
                re[l].join();
            }

            re.clear();
        }
    }

    for (int i = 0; i <= n; ++i) {
        cout << vec[i] << " " << endl;
    }

    cout << endl;

    if (n >= 2) {
        for (int j = 0; j < result.size(); ++j) {
            if (result[j] != 0) {
                cout << result[j] << " ";
            }
        }
    } else if (n == 1) {
        cout << vec[n] << " " << endl;
    }

    cout << endl;

    return 0;
}
#include <iostream>
#include <cmath>
#include <ostream>
#include <istream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string.h>
#include <sstream>
using namespace std;

class Fraction
{
public:
    friend Fraction operator+(const Fraction& frac1, const Fraction& frac2); //重载+运算符
    friend Fraction operator-(const Fraction& frac1, const Fraction& frac2); //重载-运算符
    friend Fraction operator*(const Fraction& frac1, const Fraction& frac2); //重载*运算符
    friend Fraction operator/(const Fraction& frac1, const Fraction& frac2); //重载/运算符
    friend bool operator==(Fraction frac1, Fraction frac2);                  //重载==运算符
    friend bool operator>(const Fraction& frac1, const Fraction& frac2);     //重载>运算符
    friend bool operator<(const Fraction& frac1, const Fraction& frac2);     //重载<运算符
    friend ostream& operator<<(ostream& out, const Fraction& frac);          //重载<<运算符
    friend istream& operator>>(istream& in, Fraction& frac);                 //重载>>运算符
    friend void sortFraction1(Fraction* frac, int n);                        //对分数数组升序排序
    friend void sortFraction2(Fraction* frac, int n);                        //对分数数组降序排序
    Fraction();                                                              //无参构造函数
    Fraction(int n, int d);                                                  //带参构造函数
    Fraction(const Fraction& f);                                             //复制构造函数
    void setFraction(int n, int d);                                          //设置分数的分子和分母
    int getNumer();                                                          //获取分数的分子
    int getDeno();                                                           //获取分数的分母
    void RdcFrc();                                                           //当前分数约分
private:
    int numer; //分子
    int deno;  //分母    
};

Fraction::Fraction() //无参构造函数
{
    numer = 0;
    deno = 1;
}

Fraction::Fraction(int n, int d) //带参构造函数
{
    numer = n;
    deno = d;
    RdcFrc();
}

Fraction::Fraction(const Fraction& f) //复制构造函数
{
    numer = f.numer;
    deno = f.deno;
}

void Fraction::setFraction(int n, int d) //设置分数的分子和分母
{
    numer = n;
    deno = d;
    RdcFrc();
}

int Fraction::getNumer() //获取分数的分子
{
    return numer;
}
int Fraction::getDeno() //获取分数的分母
{
    return deno;
}
void Fraction::RdcFrc() //当前分数约分
{
    int a = numer, b = deno;
    while (b != 0)
    {
        int t = a % b;
        a = b;
        b = t;
    }
    numer /= a;
    deno /= a;
}
Fraction operator+(const Fraction& frac1, const Fraction& frac2) //重载+运算符
{
    return Fraction(frac1.numer * frac2.deno + frac2.numer * frac1.deno, frac1.deno * frac2.deno);
}
Fraction operator-(const Fraction& frac1, const Fraction& frac2) //重载-运算符
{
    return Fraction(frac1.numer * frac2.deno - frac2.numer * frac1.deno, frac1.deno * frac2.deno);
}
Fraction operator*(const Fraction& frac1, const Fraction& frac2) //重载*运算符
{
    return Fraction(frac1.numer * frac2.numer, frac1.deno * frac2.deno);
}
Fraction operator/(const Fraction& frac1, const Fraction& frac2) //重载/运算符
{
    return Fraction(frac1.numer * frac2.deno, frac1.deno * frac2.numer);
}
bool operator==(Fraction frac1, Fraction frac2) //重载==运算符
{
    return frac1.numer * frac2.deno == frac2.numer * frac1.deno;
}
bool operator>(const Fraction& frac1, const Fraction& frac2) //重载>运算符
{
    return frac1.numer * frac2.deno > frac2.numer * frac1.deno;
}
bool operator<(const Fraction& frac1, const Fraction& frac2) //重载<运算符
{
    return frac1.numer * frac2.deno < frac2.numer * frac1.deno;
}
ostream& operator<<(ostream& out, const Fraction& frac) //重载<<运算符
{
    out << frac.numer << "/" << frac.deno;
    return out;
}
istream& operator>>(istream& in, Fraction& frac) //重载>>运算符
{
    in >> frac.numer >> frac.deno;
    frac.RdcFrc();
    return in;
}
void sortFraction1(Fraction* frac, int n) //对分数数组升序排序
{
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n - i - 1; j++)
        {
            if (frac[j] > frac[j + 1])
            {
                Fraction temp = frac[j];
                frac[j] = frac[j + 1];
                frac[j + 1] = temp;
            }
        }
    }
}
void sortFraction2(Fraction* frac, int n) //对分数数组降序排序
{
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n - i - 1; j++)
        {
            if (frac[j] < frac[j + 1])
            {
                Fraction temp = frac[j];
                frac[j] = frac[j + 1];
                frac[j + 1] = temp;
            }
        }
    }
}
vector<string> split(string& str, const string& delim)
{
    vector<string> res;
    if ("" == str)
    {
        return res;
    }
    char* strs = new char[str.length() + 1];
    char* d = new char[delim.length() + 1];
    strcpy(strs, str.c_str());
    strcpy(d, delim.c_str());

    char* p = strtok(strs, d);
    while (p)
    {
        string s = p;
        res.push_back(s);
        p = strtok(NULL, d);
    }
    delete[] strs;
    delete[] d;

    return res;
}

int main()
{
    while (true)
    {
        cout << "请选择功能：(键入1或者2)" << endl;
        cout << "1.分数计算" << endl;
        cout << "2.分数排序" << endl;
        cout << "——" << endl;
        int choice;
        cin >> choice;
        cin.ignore(); // 清除输入缓冲区的换行符

        if (choice == 1)
        {
            while (true)
            {
                cout << "请输入分数计算式(如:1/2+1/3),输入#返回上一层目录:" << endl;
                string str;
                getline(cin, str);
                if (str == "#")
                {
                    break;
                }
                int numer1 = -1, deno1 = -1, numer2 = -1, deno2 = -1;
                char op;
                sscanf(str.c_str(), "%d/%d%c%d/%d", &numer1, &deno1, &op, &numer2, &deno2);
                if (numer1 == -1 || deno1 == -1 || numer2 == -1 || deno2 == -1)
                {
                    cout << "输入错误!" << endl;
                    continue;
                }
                Fraction x(numer1, deno1), y(numer2, deno2);
                switch (op)
                {
                case '+':
                    cout << x + y << endl;
                    break;
                case '-':
                    cout << x - y << endl;
                    break;
                case '*':
                    cout << x * y << endl;
                    break;
                case '/':
                    cout << x / y << endl;
                    break;
                default:
                    cout << "输入错误" << endl;
                }
            }
        }
        else if (choice == 2)
        {
            while (true)
            {
                cout << "输入一组分数,用英文逗号隔开,如需由小到大排序用<结尾，由大到小排序用>结尾(如1/2,1/4,3/5<),输入#返回上层目录：" << endl;
                string formula;
                getline(cin, formula);
                if (formula == "#")
                {
                    break;
                }
                char op = formula[formula.size() - 1];
                if (op != '<' && op != '>')
                {
                    cout << "输入错误!" << endl;
                    continue;
                }
                formula[formula.size() - 1] = '\0';
                vector<string> strs = split(formula, ",");
                bool flag = true;
                vector<Fraction> frac;
                for (size_t i = 0; i < strs.size(); i++)
                {
                    int numer = -1, deno = -1;
                    sscanf(strs[i].c_str(), "%d/%d", &numer, &deno);
                    if (numer == -1 || deno == -1)
                    {
                        cout << "输入错误!" << endl;
                        flag = false;
                        break;
                    }
                    Fraction temp(numer, deno);
                    frac.push_back(temp);
                }
                if (!flag) continue;
                if (op == '<')
                    sortFraction1(frac.data(), frac.size());
                else if (op == '>')
                    sortFraction2(frac.data(), frac.size());
                for (size_t i = 0; i < frac.size(); i++)
                {
                    cout << frac[i] << " ";
                }
                cout << endl;
            }
        }
        else
        {
            cout << "输入错误，请重新选择！" << endl;
        }
    }
    return 0;
}
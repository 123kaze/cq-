#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <limits>
#include <algorithm>
#include <cmath>

using namespace std;

const int MAX_LOGIN_ATTEMPTS = 3;
const int ACCOUNT_NUMBER_LENGTH = 19;
const int ID_CARD_LENGTH = 18;
const int PASSWORD_LENGTH = 6;
const double INITIAL_BALANCE = 10000.0;
const int WITHDRAWAL_MULTIPLE = 100;
const double DAILY_WITHDRAWAL_LIMIT = 5000.0;
const double SINGLE_WITHDRAWAL_LIMIT = 2000.0;

const string ACCOUNTS_FILE = "accounts.dat";
const string TRANSACTIONS_FILE = "transactions.dat";
const string LOCKED_ACCOUNTS_FILE = "locked_accounts.dat";

struct Transaction {
    string accountNumber;
    string type;
    double amount;
    string date;
    string time;
    string targetAccount;
    
    Transaction() : amount(0.0) {}
    
    Transaction(string acc, string t, double amt, string d, string tm, string target = "")
        : accountNumber(acc), type(t), amount(amt), date(d), time(tm), targetAccount(target) {}
};

class Account {
private:
    string accountNumber;
    string name;
    string idCard;
    string password;
    double balance;
    
public:
    Account() : balance(0.0) {}
    
    Account(string accNum, string n, string id, string pwd, double bal = INITIAL_BALANCE)
        : accountNumber(accNum), name(n), idCard(id), password(pwd), balance(bal) {}
    
    string getAccountNumber() const { return accountNumber; }
    string getName() const { return name; }
    string getIdCard() const { return idCard; }
    string getPassword() const { return password; }
    double getBalance() const { return balance; }
    
    void setPassword(const string& newPwd) { password = newPwd; }
    
    bool withdraw(double amount) {
        if (amount <= 0 || amount > balance) {
            return false;
        }
        balance -= amount;
        return true;
    }
    
    bool deposit(double amount) {
        if (amount <= 0) {
            return false;
        }
        balance += amount;
        return true;
    }
    
    bool transfer(double amount, Account& targetAccount) {
        if (amount <= 0 || amount > balance) {
            return false;
        }
        balance -= amount;
        targetAccount.balance += amount;
        return true;
    }
    
    bool verifyPassword(const string& pwd) const {
        return password == pwd;
    }
    
    void display() const {
        cout << "\nAccount Information:" << endl;
        cout << "Account: " << accountNumber << endl;
        cout << "Name: " << name << endl;
        cout << "ID Card: " << idCard << endl;
        cout << "Balance: ¥" << fixed << setprecision(2) << balance << endl;
    }
    
    string toFileString() const {
        stringstream ss;
        ss << accountNumber << "," << name << "," << idCard << "," << password << "," << fixed << setprecision(2) << balance;
        return ss.str();
    }
    
    // 从文件字符串解析
    static Account fromFileString(const string& line) {
        Account account;
        stringstream ss(line);
        string token;
        
        // 解析账号
        if (getline(ss, token, ',')) account.accountNumber = token;
        if (getline(ss, token, ',')) account.name = token;
        if (getline(ss, token, ',')) account.idCard = token;
        if (getline(ss, token, ',')) account.password = token;
        if (getline(ss, token, ',')) account.balance = stod(token);
        
        return account;
    }
};

class FileManager {
public:
    static map<string, Account> loadAccounts() {
        map<string, Account> accounts;
        ifstream file(ACCOUNTS_FILE);
        
        if (file.is_open()) {
            string line;
            while (getline(file, line)) {
                Account acc = Account::fromFileString(line);
                accounts[acc.getAccountNumber()] = acc;
            }
            file.close();
        }
        
        return accounts;
    }
    
    static bool saveAccounts(const map<string, Account>& accounts) {
        ofstream file(ACCOUNTS_FILE);
        
        if (!file.is_open()) {
            return false;
        }
        
        for (const auto& pair : accounts) {
            file << pair.second.toFileString() << endl;
        }
        
        file.close();
        return true;
    }
    
    static void logTransaction(const Transaction& trans) {
        ofstream file(TRANSACTIONS_FILE, ios::app);
        
        if (file.is_open()) {
            file << trans.accountNumber << ","
                 << trans.type << ","
                 << fixed << setprecision(2) << trans.amount << ","
                 << trans.date << ","
                 << trans.time << ","
                 << trans.targetAccount << endl;
            file.close();
        }
    }
    
    static double getTodayWithdrawalTotal(const string& accountNumber) {
        double total = 0.0;
        ifstream file(TRANSACTIONS_FILE);
        
        if (!file.is_open()) {
            return 0.0;
        }
        
        // 获取当前日期
        time_t now = time(0);
        tm* localTime = localtime(&now);
        string todayDate = to_string(localTime->tm_year + 1900) + "-" +
                          to_string(localTime->tm_mon + 1) + "-" +
                          to_string(localTime->tm_mday);
        
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string token;
            vector<string> tokens;
            
            while (getline(ss, token, ',')) {
                tokens.push_back(token);
            }
            
            if (tokens.size() >= 5) {
                string acc = tokens[0];
                string type = tokens[1];
                string date = tokens[3];
                
                if (acc == accountNumber && type == "WITHDRAWAL" && date == todayDate) {
                    double amount = stod(tokens[2]);
                    total += amount;
                }
            }
        }
        
        file.close();
        return total;
    }
    
    static bool isAccountLocked(const string& accountNumber) {
        ifstream file(LOCKED_ACCOUNTS_FILE);
        
        if (!file.is_open()) {
            return false;
        }
        
        string line;
        while (getline(file, line)) {
            if (line == accountNumber) {
                file.close();
                return true;
            }
        }
        
        file.close();
        return false;
    }
    
    static void lockAccount(const string& accountNumber) {
        ofstream file(LOCKED_ACCOUNTS_FILE, ios::app);
        
        if (file.is_open()) {
            file << accountNumber << endl;
            file.close();
        }
    }
};

class ATM {
private:
    map<string, Account> accounts;
    Account* currentAccount;
    int loginAttempts;
    bool isLoggedIn;
    
public:
    ATM() : currentAccount(nullptr), loginAttempts(0), isLoggedIn(false) {
        // 加载账户数据
        accounts = FileManager::loadAccounts();
        
        // 如果没有账户数据，创建一些示例账户
        if (accounts.empty()) {
            createSampleAccounts();
        }
    }
    
    ~ATM() {
        // 保存账户数据
        FileManager::saveAccounts(accounts);
    }
    
    void createSampleAccounts() {
        Account acc1("1234567890123456789", "Zhang San", "110101199001011234", "123456", INITIAL_BALANCE);
        Account acc2("5002222005040623456", "Li Hua","500222200504062345","123456",999999);
        
        accounts[acc1.getAccountNumber()] = acc1;
        accounts[acc2.getAccountNumber()] = acc2;
        
        FileManager::saveAccounts(accounts);
    }
    
    void showWelcome() {
        cout << "\nWelcome to ATM Simulation System" << endl;
        cout << "Please insert your card (enter account number) or type 'exit' to quit" << endl;
    }
    
    bool login() {
        string accountNumber, password;
        
        cout << "\nPlease enter your 19-digit account number: ";
        cin >> accountNumber;
        
        if (accountNumber == "exit") {
            return false;
        }
        
        if (accounts.find(accountNumber) == accounts.end()) {
            cout << "Account does not exist!" << endl;
            return false;
        }
        
        if (FileManager::isAccountLocked(accountNumber)) {
            cout << "Account is locked, please contact bank customer service!" << endl;
            return false;
        }
        
        cout << "Please enter 6-digit password: ";
        cin >> password;
        
        Account& account = accounts[accountNumber];
        
        if (account.verifyPassword(password)) {
            currentAccount = &account;
            loginAttempts = 0;
            isLoggedIn = true;
            cout << "\nLogin successful! Welcome " << account.getName() << " !" << endl;
            return true;
        } else {
            loginAttempts++;
            cout << "Wrong password! Remaining attempts: " << MAX_LOGIN_ATTEMPTS - loginAttempts << endl;
            
            if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
                cout << "Too many wrong password attempts, account has been locked!" << endl;
                FileManager::lockAccount(accountNumber);
            }
            
            return false;
        }
    }
    
    void showMainMenu() {
        cout << "\nMain Menu" << endl;
        cout << "1. Check Balance" << endl;
        cout << "2. Withdraw" << endl;
        cout << "3. Deposit" << endl;
        cout << "4. Transfer" << endl;
        cout << "5. Change Password" << endl;
        cout << "6. Display Account Information" << endl;
        cout << "7. Exit/Logout" << endl;
        cout << "Please choose operation (1-7): ";
    }
    
    // 查询余额
    void checkBalance() {
        if (!isLoggedIn || !currentAccount) {
            cout << "Please login first!" << endl;
            return;
        }
        
        cout << "\nBalance Inquiry" << endl;
        cout << "Current balance: ¥" << fixed << setprecision(2) << currentAccount->getBalance() << endl;
        
        recordTransaction("BALANCE_QUERY", 0.0);
    }
    
    // 取款
    void withdraw() {
        if (!isLoggedIn || !currentAccount) {
            cout << "Please login first!" << endl;
            return;
        }
        
        double amount;
        cout << "\nWithdrawal" << endl;
        cout << "Single withdrawal limit: ¥" << SINGLE_WITHDRAWAL_LIMIT << endl;
        cout << "Daily withdrawal limit: ¥" << DAILY_WITHDRAWAL_LIMIT << endl;
        cout << "Withdrawal amount must be multiple of " << WITHDRAWAL_MULTIPLE << endl;
        
        double todayTotal = FileManager::getTodayWithdrawalTotal(currentAccount->getAccountNumber());
        cout << "Today's withdrawals: ¥" << todayTotal << endl;
        
        cout << "Please enter withdrawal amount: ";
        cin >> amount;
        
        if (cin.fail() || amount <= 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid amount!" << endl;
            return;
        }
        
        if (fmod(amount, WITHDRAWAL_MULTIPLE) != 0) {
            cout << "Withdrawal amount must be multiple of " << WITHDRAWAL_MULTIPLE << "!" << endl;
            return;
        }
        
        if (amount > SINGLE_WITHDRAWAL_LIMIT) {
            cout << "Exceeds single withdrawal limit!" << endl;
            return;
        }
        
        if (todayTotal + amount > DAILY_WITHDRAWAL_LIMIT) {
            cout << "Exceeds daily withdrawal limit!" << endl;
            return;
        }
        
        if (amount > currentAccount->getBalance()) {
            cout << "Insufficient balance!" << endl;
            return;
        }
        
        if (currentAccount->withdraw(amount)) {
            cout << "Withdrawal successful! Withdrawn amount: ¥" << amount << endl;
            cout << "Remaining balance: ¥" << currentAccount->getBalance() << endl;
            
            recordTransaction("WITHDRAWAL", amount);
            
            FileManager::saveAccounts(accounts);
        } else {
            cout << "Withdrawal failed!" << endl;
        }
    }
    
    // 存款
    void deposit() {
        if (!isLoggedIn || !currentAccount) {
            cout << "Please login first!" << endl;
            return;
        }
        
        double amount;
        cout << "\nDeposit" << endl;
        cout << "Please enter deposit amount: ";
        cin >> amount;
        
        if (cin.fail() || amount <= 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid amount!" << endl;
            return;
        }
        
        if (currentAccount->deposit(amount)) {
            cout << "Deposit successful! Deposit amount: ¥" << amount << endl;
            cout << "Current balance: ¥" << currentAccount->getBalance() << endl;
            
            recordTransaction("DEPOSIT", amount);
            
            FileManager::saveAccounts(accounts);
        } else {
            cout << "Deposit failed!" << endl;
        }
    }
    
    // 转账
    void transfer() {
        if (!isLoggedIn || !currentAccount) {
            cout << "Please login first!" << endl;
            return;
        }
        
        string targetAccountNumber;
        double amount;
        
        cout << "\nTransfer" << endl;
        cout << "Please enter target account number: ";
        cin >> targetAccountNumber;
        
        if (accounts.find(targetAccountNumber) == accounts.end()) {
            cout << "Target account does not exist!" << endl;
            return;
        }
        
        if (targetAccountNumber == currentAccount->getAccountNumber()) {
            cout << "Cannot transfer to yourself!" << endl;
            return;
        }
        
        string confirmAccount;
        cout << "Please re-enter target account number to confirm: ";
        cin >> confirmAccount;
        
        if (targetAccountNumber != confirmAccount) {
            cout << "Two account numbers do not match!" << endl;
            return;
        }
        
        cout << "Please enter transfer amount: ";
        cin >> amount;
        
        if (cin.fail() || amount <= 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid amount!" << endl;
            return;
        }
        
        if (amount > currentAccount->getBalance()) {
            cout << "Insufficient balance!" << endl;
            return;
        }
        
        Account& targetAccount = accounts[targetAccountNumber];
        
        if (currentAccount->transfer(amount, targetAccount)) {
            cout << "Transfer successful! Transfer amount: ¥" << amount << endl;
            cout << "Remaining balance: ¥" << currentAccount->getBalance() << endl;
            cout << "Recipient: " << targetAccount.getName() << endl;
            
            recordTransaction("TRANSFER", amount, targetAccountNumber);
            
            FileManager::saveAccounts(accounts);
        } else {
            cout << "Transfer failed!" << endl;
        }
    }
    
    // 修改密码
    void changePassword() {
        if (!isLoggedIn || !currentAccount) {
            cout << "Please login first!" << endl;
            return;
        }
        
        string oldPassword, newPassword, confirmPassword;
        
        cout << "\nChange Password" << endl;
        cout << "Please enter current password: ";
        cin >> oldPassword;
        
        if (!currentAccount->verifyPassword(oldPassword)) {
            cout << "Current password is incorrect!" << endl;
            return;
        }
        
        cout << "Please enter new password (6 digits): ";
        cin >> newPassword;
        
        if (newPassword.length() != PASSWORD_LENGTH) {
            cout << "Password must be 6 digits!" << endl;
            return;
        }
        
        if (!all_of(newPassword.begin(), newPassword.end(), ::isdigit)) {
            cout << "Password must be numeric!" << endl;
            return;
        }
        
        cout << "Please re-enter new password to confirm: ";
        cin >> confirmPassword;
        
        if (newPassword != confirmPassword) {
            cout << "Two passwords do not match!" << endl;
            return;
        }
        
        currentAccount->setPassword(newPassword);
        
        FileManager::saveAccounts(accounts);
        
        cout << "Password changed successfully!" << endl;
    }
    
    // 显示账户信息
    void displayAccountInfo() {
        if (!isLoggedIn || !currentAccount) {
            cout << "Please login first!" << endl;
            return;
        }
        
        currentAccount->display();
    }
    
    void logout() {
        if (isLoggedIn) {
            cout << "\nThank you for using, welcome next time!" << endl;
            currentAccount = nullptr;
            isLoggedIn = false;
        }
    }
    
    void run() {
        showWelcome();
        
        while (!isLoggedIn) {
            if (!login()) {
                if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
                    cout << "Too many login failures, program exits." << endl;
                    return;
                }
                
                char choice;
                cout << "\nContinue to try login? (y/n): ";
                cin >> choice;
                
                if (choice != 'y' && choice != 'Y') {
                    return;
                }
            }
        }
        
        int choice;
        while (isLoggedIn) {
            showMainMenu();
            cin >> choice;
            
            switch (choice) {
                case 1: checkBalance(); break;
                case 2: withdraw(); break;
                case 3: deposit(); break;
                case 4: transfer(); break;
                case 5: changePassword(); break;
                case 6: displayAccountInfo(); break;
                case 7: logout(); break;
                default:
                    cout << "Invalid choice, please re-enter!" << endl;
                    break;
            }
            
            if (choice != 7) {
                cout << "\nPress any key to continue...";
                cin.ignore();
                cin.get();
            }
        }
    }
    
private:
    // 记录交易
    void recordTransaction(const string& type, double amount, const string& targetAccount = "") {
        // 获取当前日期和时间
        time_t now = time(0);
        tm* localTime = localtime(&now);
        
        string date = to_string(localTime->tm_year + 1900) + "-" +
                     to_string(localTime->tm_mon + 1) + "-" +
                     to_string(localTime->tm_mday);
        
        string time = to_string(localTime->tm_hour) + ":" +
                     to_string(localTime->tm_min) + ":" +
                     to_string(localTime->tm_sec);
        
        Transaction trans(currentAccount->getAccountNumber(), type, amount, date, time, targetAccount);
        FileManager::logTransaction(trans);
    }
};

// ====================== 主函数 ======================
int main() {
    // 设置控制台为UTF-8编码（Windows）
    #ifdef _WIN32
        system("chcp 65001 > nul");
    #endif
    
    ATM atm;
    
    try {
        atm.run();
    } catch (const exception& e) {
        cerr << "发生错误: " << e.what() << endl;
        return 1;
    }
    
    cout << "\n程序结束。" << endl;
    return 0;
}

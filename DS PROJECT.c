//Library function//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FINE_PER_DAY 5

// ---------------------------- UTIL: FILE EXIST ------------------------//
void ensureFileExists(const char* fname) {
    FILE* fp = fopen(fname, "ab");
    if(fp) fclose(fp);
}

//-----------------------------book management----------------------------//
struct Book {
    int id;
    char title[50];
    char author[50];
    char category[30];
    int edition;
    int totalCopies;
    int available;
    struct Book* next;
};
//user management structure//
struct User {
    char id[20]; // e.g., 251-15-455
    char name[50];
    char type[20]; // Student/Teacher
    char department[30];
    struct User* next;
};
//global variable//
struct User* userHead = NULL;

// issue date function//
struct Issue {
    int bookId;
    char userId[20];
    struct tm issueDate;
    struct tm returnDate;
    int fine;           // calculated fine
    int fineCleared;    // 0 = not cleared, 1 = cleared
    struct Issue* next;
};

// Stack for recent issues//
struct StackNode {
    int bookId;
    struct StackNode* next;
};
// global variable //
struct Book* bookHead = NULL;
struct Issue* issueHead = NULL;
struct StackNode* stackTop = NULL;

//stack function//
void push(int bookId) {
    struct StackNode* node = (struct StackNode*)malloc(sizeof(struct StackNode));
    if(!node) return;
    node->bookId = bookId;
    node->next = stackTop;
    stackTop = node;
}

void viewStack() {
    struct StackNode* temp = stackTop;
    printf("\n--- RECENTLY ISSUED BOOKS ---\n");
    if(!temp) { printf("No recent issues.\n"); return; }
    while(temp) {
        struct Book* b = bookHead;
        while(b && b->id != temp->bookId) b=b->next;
        if(b) printf("Book ID: %d | Title: %s\n", b->id, b->title);
        temp = temp->next;
    }
}
// date helpers//
void formatDate(struct tm date, char *buf, size_t sz) {
    if(date.tm_year==0) {
        snprintf(buf, sz, "Not Returned");
    } else {
        strftime(buf, sz, "%d/%m/%Y", &date);
    }
}

struct tm inputDate() {
    struct tm date = {0};
    int d,m,y;
    // We assume user will enter correctly; basic scanf
    scanf("%d/%d/%d",&d,&m,&y);
    date.tm_mday = d;
    date.tm_mon = m-1;
    date.tm_year = y-1900;
    return date;
}

int calculateFine(struct tm issue, struct tm ret) {
    time_t t_issue = mktime(&issue);
    time_t t_ret = mktime(&ret);
    double diff = difftime(t_ret,t_issue);
    int days = (int)(diff/(60*60*24));
    if(days>7) return (days-7)*FINE_PER_DAY;
    return 0;
}

//file handling //
void saveBooks() {
    ensureFileExists("books.dat");
    FILE* fp = fopen("books.dat","wb");
    if(!fp) { perror("books.dat open for write failed"); return; }
    struct Book* temp = bookHead;
    while(temp) {
        fwrite(temp,sizeof(struct Book),1,fp);
        temp = temp->next;
    }
    fclose(fp);
}

void loadBooks() {
    ensureFileExists("books.dat");
    FILE* fp = fopen("books.dat","rb");
    if(!fp) return;
    struct Book* last=NULL;
    while(1) {
        struct Book* b = (struct Book*)malloc(sizeof(struct Book));
        if(!b) break;
        if(fread(b,sizeof(struct Book),1,fp)!=1) {
            free(b); break;
        }
        b->next=NULL;
        if(!bookHead) bookHead=b;
        else last->next=b;
        last=b;
    }
    fclose(fp);
}
// issue //
void saveIssues() {
    ensureFileExists("issues.dat");
    FILE* fp = fopen("issues.dat","wb");
    if(!fp) { perror("issues.dat open for write failed"); return; }
    struct Issue* temp = issueHead;
    while(temp) {
        fwrite(temp,sizeof(struct Issue),1,fp);
        temp = temp->next;
    }
    fclose(fp);
}

void loadIssues() {
    ensureFileExists("issues.dat");
    FILE* fp = fopen("issues.dat","rb");
    if(!fp) return;
    struct Issue* last=NULL;
    while(1) {
        struct Issue* I = (struct Issue*)malloc(sizeof(struct Issue));
        if(!I) break;
        if(fread(I,sizeof(struct Issue),1,fp)!=1) {
            free(I); break;
        }
        I->next=NULL;
        if(!issueHead) issueHead=I;
        else last->next=I;
        last=I;
    }
    fclose(fp);
}

// all called function in book management//
void addBook() {
    struct Book* b = (struct Book*)malloc(sizeof(struct Book));

    printf("\nEnter Book ID: ");
    scanf("%d", &b->id);

    // Check if book ID already exists
    struct Book* temp = bookHead;
    while(temp) {
        if(temp->id == b->id) {
            printf("Book with ID %d already exists!\n", b->id);
            free(b);
            return;
        }
        temp = temp->next;
    }

    printf("Title: ");
    scanf(" %[^\n]", b->title);
    printf("Author: ");
    scanf(" %[^\n]", b->author);
    printf("Category: ");
    scanf(" %[^\n]", b->category);
    printf("Edition: ");
    scanf("%d", &b->edition);
    printf("Total Copies: ");
    scanf("%d", &b->totalCopies);

    b->available = b->totalCopies;
    b->next = bookHead;
    bookHead = b;

    saveBooks();
    printf("Book Added!\n");
}

void viewBooks() {
    struct Book* b = bookHead;

    printf("\n===== BOOK LIST =====\n");
    if(!b) {
        printf("No books found.\n");
        return;
    }

    while(b) {
        printf("ID: %d | Title: %s | Author: %s | Category: %s | Edition: %d | Available: %d/%d\n",
               b->id, b->title, b->author, b->category, b->edition, b->available, b->totalCopies);
        b = b->next;
    }
}


void removeBook() {
    int id;
    printf("\nEnter Book ID to remove: ");
    scanf("%d",&id);
    struct Book* temp=bookHead;
    struct Book* prev=NULL;
    while(temp && temp->id!=id) { prev=temp; temp=temp->next; }
    if(!temp) { printf("Book not found!\n"); return; }
    if(prev) prev->next=temp->next;
    else bookHead=temp->next;
    free(temp);
    saveBooks();
    printf("Book Removed!\n");
}

// issue & return //
void issueBook() {
    int bookId;
    char userId[20];
    printf("\nEnter Book ID: ");
    if(scanf("%d",&bookId)!=1){ int c; while((c=getchar())!=EOF && c!='\n'); printf("Invalid input\n"); return;}
    printf("Enter User ID: ");
    scanf(" %[^\n]",userId);

    int totalPending = totalPendingFineAmount(userId);
    if(totalPending>0){
        printf("Cannot issue new book. User has pending dues: %d tk. Clear dues first.\n", totalPending);
        return;
    }

    struct Book* b=bookHead;
    while(b && b->id!=bookId) b=b->next;
    if(!b) { printf("Book not found!\n"); return; }
    if(b->available==0) { printf("No copies available!\n"); return; }
    b->available--;

    struct Issue* I=(struct Issue*)malloc(sizeof(struct Issue));
    if(!I){ printf("Memory error\n"); return; }
    I->bookId=bookId;
    strcpy(I->userId,userId);
    printf("Enter Issue Date (DD/MM/YYYY): ");
    I->issueDate=inputDate();
    I->returnDate=(struct tm){0};
    I->fine=0;
    I->fineCleared=0;
    I->next=issueHead;
    issueHead=I;

    push(bookId);
    saveIssues();
    saveBooks();
    printf(" Book Issued!\n");
}

void returnBook() {
    int bookId;
    char userId[20];
    printf("\nEnter Book ID: ");
    if(scanf("%d",&bookId)!=1){ int c; while((c=getchar())!=EOF && c!='\n'); printf("Invalid input\n"); return;}
    printf("Enter User ID: ");
    scanf(" %[^\n]",userId);

    struct Issue* I=issueHead;
    while(I){
        if(I->bookId==bookId && strcmp(I->userId,userId)==0 && I->returnDate.tm_year==0){
            printf("Enter Return Date (DD/MM/YYYY): ");
            I->returnDate=inputDate();
            I->fine=calculateFine(I->issueDate,I->returnDate);
            I->fineCleared = 0;

            struct Book* b=bookHead;
            while(b && b->id!=bookId) b=b->next;
            if(b) b->available++;

            saveIssues();
            saveBooks();
            printf("Book Returned! Due Fine: %d tk\n",I->fine);
            return;
        }
        I=I->next;
    }
    printf(" No matching issue record found!\n");
}

void viewIssued() {
    struct Issue* I=issueHead;
    printf("\n--- ISSUED BOOKS ---\n");
    int any = 0;
    while(I){
        struct Book* b=bookHead;
        while(b && b->id!=I->bookId) b=b->next;
        struct User* u=userHead;
        while(u && strcmp(u->id,I->userId)!=0) u=u->next;
        if(b && u){
            any = 1;
            char issueBuf[20], returnBuf[20];
            formatDate(I->issueDate, issueBuf, sizeof(issueBuf));
            formatDate(I->returnDate, returnBuf, sizeof(returnBuf));

            if(I->fineCleared)
                printf("Book ID: %d | Title: %s | User: %s | Dept: %s | Issue: %s | Return: %s | Due Fine: %d tk (Cleared)\n",
                    b->id,b->title,u->name,u->department,issueBuf,returnBuf,I->fine);
            else
                printf("Book ID: %d | Title: %s | User: %s | Dept: %s | Issue: %s | Return: %s | Due Fine: %d tk\n",
                    b->id,b->title,u->name,u->department,issueBuf,returnBuf,I->fine);
        }
        I=I->next;
    }
    if(!any) printf("No issue records.\n");
}

// main menu for book management //
void bookManagement(){
    int ch;
    while(1){
        printf("\n===== BOOK MANAGEMENT =====\n");
        printf("1. Add Book\n");
        printf("2. Remove Book\n");
        printf("3. View Books\n");
        printf("4. Issue Book\n");
        printf("5. Return Book\n");
        printf("6. View Issued Books\n");
        printf("7. Recent Issues (Stack)\n");
        printf("0. Back\n");
        printf("Enter choice: ");
        scanf("%d",&ch);

        switch(ch){
            case 1: addBook(); break;
            case 2: removeBook(); break;
            case 3: viewBooks(); break;
            case 4: issueBook(); break;
            case 5: returnBook(); break;
            case 6: viewIssued(); break;
            case 7: viewStack(); break;
            case 0: return;
            default: printf("Invalid choice!\n");
        }
    }
}

//------------------------User management--------------------------//
/*
//user management structure//
struct User {
    char id[20]; // e.g., 251-15-455
    char name[50];
    char type[20]; // Student/Teacher
    char department[30];
    struct User* next;
};
//global variable//
struct User* userHead = NULL;
*/

//file handling//
void saveUsers() {
    ensureFileExists("users.dat");
    FILE* fp = fopen("users.dat","wb");
    if(!fp) { perror("users.dat open for write failed"); return; }
    struct User* temp = userHead;
    while(temp) {
        fwrite(temp,sizeof(struct User),1,fp);
        temp = temp->next;
    }
    fclose(fp);
}

void loadUsers() {
    ensureFileExists("users.dat");
    FILE* fp = fopen("users.dat","rb");
    if(!fp) return;
    struct User* last=NULL;
    while(1) {
        struct User* u = (struct User*)malloc(sizeof(struct User));
        if(!u) break;
        if(fread(u,sizeof(struct User),1,fp)!=1) {
            free(u); break;
        }
        u->next=NULL;
        if(!userHead) userHead=u;
        else last->next=u;
        last=u;
    }
    fclose(fp);
}

//called function in user management function //
void addUser() {
    struct User* u = (struct User*)malloc(sizeof(struct User));

    printf("\nEnter User ID (e.g., 251-15-455): ");
    scanf(" %[^\n]", u->id);

    // Check if user ID already exists
    struct User* temp = userHead;
    while(temp) {
        if(strcmp(temp->id, u->id) == 0) {
            printf("User with ID %s already exists!\n", u->id);
            free(u);
            return;
        }
        temp = temp->next;
    }

    printf("Name: ");
    scanf(" %[^\n]", u->name);
    printf("Type (Student/Teacher): ");
    scanf(" %[^\n]", u->type);
    printf("Department: ");
    scanf(" %[^\n]", u->department);

    u->next = userHead;
    userHead = u;

    saveUsers();
    printf("User Added!\n");
}


void removeUser() {
    char id[20];
    printf("\nEnter User ID to remove: ");
    scanf(" %[^\n]",id);
    struct User* temp=userHead;
    struct User* prev=NULL;
    while(temp && strcmp(temp->id,id)!=0) {
        prev=temp; temp=temp->next;
    }
    if(!temp) { printf("User not found!\n"); return; }
    if(prev) prev->next=temp->next;
    else userHead=temp->next;
    free(temp);
    saveUsers();
    printf("User Removed!\n");
}

void showAllUsers() {
    struct User* u=userHead;
    printf("\n----- USER LIST -----\n");
    if(!u) { printf("No users found.\n"); return; }
    while(u){
        printf("ID: %s | Name: %s | Type: %s | Dept: %s\n", u->id, u->name, u->type, u->department);
        u=u->next;
    }
}

void viewIndividualUser() {
    char id[20];
    printf("Enter User ID: ");
    scanf(" %[^\n]",id);
    struct User* u=userHead;
    while(u && strcmp(u->id,id)!=0) u=u->next;
    if(!u) { printf("User not found!\n"); return; }
    printf("\nID: %s | Name: %s | Type: %s | Dept: %s\n", u->id, u->name, u->type, u->department);
    struct Issue* I=issueHead;
    printf("\n--- Borrowed Books ---\n");
    int found = 0;
    while(I){
        if(strcmp(I->userId,u->id)==0){
            struct Book* b=bookHead;
            while(b && b->id!=I->bookId) b=b->next;
            if(b){
                found = 1;
                char issueBuf[20], returnBuf[20];
                formatDate(I->issueDate, issueBuf, sizeof(issueBuf));
                formatDate(I->returnDate, returnBuf, sizeof(returnBuf));

                if(I->fineCleared)
                    printf("Book ID: %d | Title: %s | Issue Date: %s | Return Date: %s | Due Fine: %d tk (Cleared)\n",
                        b->id, b->title, issueBuf, returnBuf, I->fine);
                else
                    printf("Book ID: %d | Title: %s | Issue Date: %s | Return Date: %s | Due Fine: %d tk\n",
                        b->id, b->title, issueBuf, returnBuf, I->fine);
            }
        }
        I=I->next;
    }
    if(!found) printf("No borrowed books.\n");
}

//fine and clear //
int hasPendingFine(char* userId) {
    struct Issue* I = issueHead;
    while(I){
        if(strcmp(I->userId,userId)==0 && I->fine>0 && !I->fineCleared) return 1;
        I=I->next;
    }
    return 0;
}

int totalPendingFineAmount(char* userId) {
    struct Issue* I = issueHead;
    int total = 0;
    while(I){
        if(strcmp(I->userId,userId)==0 && I->fine>0 && !I->fineCleared) total += I->fine;
        I=I->next;
    }
    return total;
}

void clearFine() {
    char userId[20];
    printf("\nEnter User ID to clear dues: ");
    scanf(" %[^\n]", userId);

    struct Issue* I = issueHead;
    int found = 0;
    int collected = 0;
    while(I) {
        if(strcmp(I->userId, userId) == 0 && I->fine > 0 && !I->fineCleared && I->returnDate.tm_year != 0) {
            I->fineCleared = 1;
            collected += I->fine;
            found = 1;
        }
        I = I->next;
    }

    if(found) {
        saveIssues();
        printf("All pending dues for user %s have been cleared. Total collected: %d tk\n", userId, collected);
    } else {
        printf("No pending dues found for user %s.\n", userId);
    }
}

//main menu for user management //
void userManagement(){
    int ch;
    while(1){
        printf("\n===== USER MANAGEMENT =====\n");
        printf("1. Add User\n");
        printf("2. Remove User\n");
        printf("3. View Users\n");
        printf("4. Clear User Dues\n");
        printf("0. Back\n");
        printf("Enter choice: ");
        scanf("%d",&ch);

        switch(ch){
            case 1: addUser(); break;
            case 2: removeUser(); break;
            case 3:
                printf("1. Show All Users\n2. Show Individual User\nEnter choice: ");
                {
                    int c; scanf("%d",&c);
                    if(c==1) showAllUsers();
                    else if(c==2) viewIndividualUser();
                    else printf("Invalid choice!\n");
                }
                break;
            case 4: clearFine(); break;
            case 0: return;
            default: printf("Invalid choice!\n");
        }
    }
}
// ---------------------------- DASHBOARD ------------------------------
void dashboard() {
    int totalBooks=0,totalUsers=0,totalIssued=0,overdue=0,totalFine=0,totalDue=0;
    struct Book* b=bookHead;
    while(b){ totalBooks++; b=b->next; }
    struct User* u=userHead;
    while(u){ totalUsers++; u=u->next; }
    struct Issue* I=issueHead;
    printf("\n===== ADMIN DASHBOARD =====\n");
    while(I){
        if(I->returnDate.tm_year==0) totalIssued++;
        if(I->fine>0){
            if(I->fineCleared) totalFine += I->fine;
            else {
                totalDue += I->fine;
                overdue++;
            }
        }
        I=I->next;
    }
    printf("Total Books: %d\nTotal Users: %d\nTotal Issued: %d\n", totalBooks,totalUsers,totalIssued);
    printf("Overdue Books (%d): ", overdue);
    I=issueHead;
    while(I){
        if(I->fine>0 && !I->fineCleared){
            printf("[Book ID:%d, Fine:%d tk] ", I->bookId, I->fine);
        }
        I=I->next;
    }
    printf("\nTotal Pending Due: %d tk\nFine Collection (Cleared): %d tk\n", totalDue, totalFine);
}

void loadAllData() {
    loadBooks();
    loadUsers();
    loadIssues();
}

// ---------------------------- ADMIN PANEL ----------------------------
void adminPanel(){
    int ch;
    while(1){
        printf("\n===== ADMIN PANEL =====\n");
        printf("1. Book Management\n");
        printf("2. User Management\n");
        printf("3. Dashboard\n");
        printf("0. Back to Home\n");
        printf("Enter choice: ");
        scanf("%d",&ch);

        if(ch==1) bookManagement();
        else if(ch==2) userManagement();
        else if(ch==3) dashboard();
        else if(ch==0) break;
        else printf("Invalid choice!\n");
    }
}

// ---------------------------- USER PANEL -----------------------------
void userPanel() {
    char uid[20];
    printf("\nEnter your User ID: ");
    scanf(" %[^\n]", uid);

    struct User* u = userHead;
    while(u && strcmp(u->id, uid) != 0) u = u->next;
    if(!u) { printf("Invalid User ID!\n"); return; }

    int choice;
    while(1) {
        printf("\n===== USER PANEL =====\n");
        printf("Logged in as: %s (%s)\n", u->name, u->type);
        printf("1. View My Profile\n");
        printf("2. View My Borrowed Books\n");
        printf("3. View My Due Fines\n");
        printf("0. Back to Home\n");
        printf("Enter choice: ");
        if(scanf("%d", &choice)!=1){ int c; while((c=getchar())!=EOF && c!='\n'); printf("Invalid input\n"); continue; }

        if(choice == 1) {
            printf("\n--- MY PROFILE ---\n");
            printf("ID: %s\nName: %s\nType: %s\nDept: %s\n",
                   u->id, u->name, u->type, u->department);
        }
        else if(choice == 2) {
            printf("\n--- MY BORROWED BOOKS ---\n");
            struct Issue* I = issueHead;
            int found = 0;
            while(I) {
                if(strcmp(I->userId, u->id) == 0) {
                    found = 1;
                    struct Book* b = bookHead;
                    while(b && b->id != I->bookId) b = b->next;
                    if(b) {
                        char issueBuf[20], returnBuf[20];
                        formatDate(I->issueDate, issueBuf, sizeof(issueBuf));
                        formatDate(I->returnDate, returnBuf, sizeof(returnBuf));

                        if(I->fineCleared)
                            printf("Book: %s | Issue: %s | Return: %s | Fine: %d tk (Cleared)\n",
                                b->title, issueBuf, returnBuf, I->fine);
                        else
                            printf("Book: %s | Issue: %s | Return: %s | Fine: %d tk\n",
                                b->title, issueBuf, returnBuf, I->fine);
                    }
                }
                I = I->next;
            }
            if(!found) printf("No books borrowed.\n");
        }
        else if(choice == 3) {
            printf("\n--- MY DUE FINES ---\n");
            struct Issue* I = issueHead;
            int due = 0, cleared = 0;
            while(I){
                if(strcmp(I->userId,u->id)==0){
                    if(I->fine>0 && !I->fineCleared) due += I->fine;
                    if(I->fine>0 && I->fineCleared) cleared += I->fine;
                }
                I = I->next;
            }
            printf("Pending Dues: %d tk\nCleared: %d tk\n", due, cleared);
        }
        else if(choice == 0) {
            return;
        }
        else printf("Invalid choice!\n");
    }
}

// ---------------------------- SEARCH BOOKS ---------------------------//
void searchBooks() {
    int choice;
    char text[100];
    int id;

    printf("\n===== BOOK SEARCH PANEL =====\n");
    printf("1. Search by Book ID\n");
    printf("2. Search by Title\n");
    printf("3. Search by Author\n");
    printf("4. Search by Category\n");
    printf("Enter choice: ");
    if(scanf("%d", &choice)!=1){ int c; while((c=getchar())!=EOF && c!='\n'); printf("Invalid input\n"); return; }

    struct Book* b = bookHead;
    int found = 0;

    if(choice == 1) {
        printf("Enter Book ID: ");
        scanf("%d", &id);
        while(b) {
            if(b->id == id) {
                found = 1;
                printf("\n[BOOK FOUND]\nID: %d\nTitle: %s\nAuthor: %s\nCategory: %s\nEdition: %d\nAvailable: %d/%d\n",
                       b->id, b->title, b->author, b->category, b->edition, b->available, b->totalCopies);
                break;
            }
            b = b->next;
        }
    }
    else {
        printf("Enter text (partial allowed): ");
        scanf(" %[^\n]", text);

        while(b) {
            if((choice == 2 && strstr(b->title, text)) ||
               (choice == 3 && strstr(b->author, text)) ||
               (choice == 4 && strstr(b->category, text))) {

                found = 1;
                printf("\n[BOOK FOUND]\nID: %d\nTitle: %s\nAuthor: %s\nCategory: %s\nEdition: %d\nAvailable: %d/%d\n",
                       b->id, b->title, b->author, b->category, b->edition, b->available, b->totalCopies);
            }
            b = b->next;
        }
    }

    if(!found) {
        printf("\nNo matching books found!\n");
    }
}
// ---------------------------- MAIN MENU ------------------------------
void mainMenu() {
    int choice;
    while(1) {
        printf("===== LIBRARY MANAGEMENT SYSTEM =====\n\n");
        printf("1. Admin Panel\n");
        printf("2. User Panel\n");
        printf("3. Search Books\n");
        printf("0. Exit\n");
        printf("Enter choice: ");
        if(scanf("%d",&choice)!=1){ // clear bad input
            int c; while((c=getchar())!=EOF && c!='\n'); continue;
        }

        switch(choice) {
            case 1: adminPanel(); break;
            case 2: userPanel(); break;
            case 3: searchBooks(); break;
            case 0:
                printf("Exiting... Saving data...\n");
                saveIssues(); saveUsers(); saveBooks();
                printf("Goodbye!\n");
                exit(0);
            default: printf("Invalid choice! Try again.\n");
        }
    }
}

// ---------------------------- MAIN -----------------------------------
int main() {
    loadAllData();
    mainMenu();
    return 0;
}



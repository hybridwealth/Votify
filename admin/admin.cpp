#include <QApplication>
#include <QMainWindow>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFormLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDateEdit>
#include <QTextEdit>

class VerifyUsersPage : public QWidget {
    Q_OBJECT
public:
    VerifyUsersPage(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *titleLabel = new QLabel("Verify Users", this);
        titleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(titleLabel);

        userListWidget = new QListWidget(this);
        layout->addWidget(userListWidget);

        loadUsers();
    }

public slots:
    void handleVerifyUser() {
        QListWidgetItem *currentItem = userListWidget->currentItem();
        if (!currentItem) {
            QMessageBox::warning(this, "Error", "No user selected.");
            return;
        }

        int userId = currentItem->data(Qt::UserRole).toInt();
        QSqlQuery query;
        query.prepare("UPDATE profiles SET isVerified = 1 WHERE id = :id");
        query.bindValue(":id", userId);
        if (!query.exec()) {
            QMessageBox::critical(this, "Error", query.lastError().text());
            return;
        }

        currentItem->setText(currentItem->text() + " (Verified)");
        QMessageBox::information(this, "Success", "User verified successfully!");
    }

    void handleBanUser() {
        QListWidgetItem *currentItem = userListWidget->currentItem();
        if (!currentItem) {
            QMessageBox::warning(this, "Error", "No user selected.");
            return;
        }

        int userId = currentItem->data(Qt::UserRole).toInt();
        QSqlQuery query;
        query.prepare("DELETE FROM profiles WHERE id = :id");
        query.bindValue(":id", userId);
        if (!query.exec()) {
            QMessageBox::critical(this, "Error", query.lastError().text());
            return;
        }

        delete currentItem;
        QMessageBox::information(this, "Success", "User banned successfully!");
    }

private:
    QListWidget *userListWidget;

    void loadUsers() {
        QSqlQuery query("SELECT id, name, dob, isVerified FROM profiles");
        while (query.next()) {
            QString user = query.value("name").toString() + " (" + query.value("dob").toString() + ")";
            if (query.value("isVerified").toBool()) {
                user += " (Verified)";
            }
            QListWidgetItem *item = new QListWidgetItem(user, userListWidget);
            item->setData(Qt::UserRole, query.value("id"));
        }
    }
};

class ManageCandidatesPage : public QWidget {
    Q_OBJECT
public:
    ManageCandidatesPage(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *titleLabel = new QLabel("Manage Candidates", this);
        titleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(titleLabel);

        QFormLayout *formLayout = new QFormLayout();
        candidateEdit = new QLineEdit(this);
        formLayout->addRow("New Candidate:", candidateEdit);
        layout->addLayout(formLayout);

        QPushButton *addCandidateButton = new QPushButton("Add Candidate", this);
        connect(addCandidateButton, &QPushButton::clicked, this, &ManageCandidatesPage::handleAddCandidate);
        layout->addWidget(addCandidateButton);

        candidateListWidget = new QListWidget(this);
        layout->addWidget(candidateListWidget);

        loadCandidates();
    }

public slots:
    void handleAddCandidate() {
        QString candidateName = candidateEdit->text();
        if (candidateName.isEmpty()) {
            QMessageBox::warning(this, "Error", "Candidate name cannot be empty.");
            return;
        }

        QSqlQuery query;
        query.prepare("INSERT INTO candidates (name) VALUES (:name)");
        query.bindValue(":name", candidateName);
        if (!query.exec()) {
            QMessageBox::critical(this, "Error", query.lastError().text());
            return;
        }

        QListWidgetItem *item = new QListWidgetItem(candidateName, candidateListWidget);
        item->setData(Qt::UserRole, query.lastInsertId());
        candidateEdit->clear();
    }

    void handleDeleteCandidate() {
        QListWidgetItem *currentItem = candidateListWidget->currentItem();
        if (!currentItem) {
            QMessageBox::warning(this, "Error", "No candidate selected.");
            return;
        }

        int candidateId = currentItem->data(Qt::UserRole).toInt();
        QSqlQuery query;
        query.prepare("DELETE FROM candidates WHERE id = :id");
        query.bindValue(":id", candidateId);
        if (!query.exec()) {
            QMessageBox::critical(this, "Error", query.lastError().text());
            return;
        }

        delete currentItem;
    }

private:
    QLineEdit *candidateEdit;
    QListWidget *candidateListWidget;

    void loadCandidates() {
        QSqlQuery query("SELECT id, name FROM candidates");
        while (query.next()) {
            QString candidateName = query.value("name").toString();
            QListWidgetItem *item = new QListWidgetItem(candidateName, candidateListWidget);
            item->setData(Qt::UserRole, query.value("id"));
        }
    }
};

class AdminMainWindow : public QMainWindow {
    Q_OBJECT
public:
    AdminMainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        QStackedWidget *stackedWidget = new QStackedWidget(this);

        VerifyUsersPage *verifyUsersPage = new VerifyUsersPage(this);
        ManageCandidatesPage *manageCandidatesPage = new ManageCandidatesPage(this);

        stackedWidget->addWidget(verifyUsersPage);
        stackedWidget->addWidget(manageCandidatesPage);

        setCentralWidget(stackedWidget);

        QMenuBar *menuBar = new QMenuBar(this);
        QMenu *menu = new QMenu("Menu", this);
        QAction *verifyUsersAction = new QAction("Verify Users", this);
        QAction *manageCandidatesAction = new QAction("Manage Candidates", this);

        connect(verifyUsersAction, &QAction::triggered, [stackedWidget, verifyUsersPage]() {
            stackedWidget->setCurrentWidget(verifyUsersPage);
        });

        connect(manageCandidatesAction, &QAction::triggered, [stackedWidget, manageCandidatesPage]() {
            stackedWidget->setCurrentWidget(manageCandidatesPage);
        });

        menu->addAction(verifyUsersAction);
        menu->addAction(manageCandidatesAction);

        menuBar->addMenu(menu);
        setMenuBar(menuBar);

        setupDatabase();
    }

private:
    void setupDatabase() {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("VotingAppDB.sqlite");

        if (!db.open()) {
            QMessageBox::critical(this, "Error", "Failed to open database.");
            return;
        }

        QSqlQuery query;
        query.exec("CREATE TABLE IF NOT EXISTS profiles (id INTEGER PRIMARY KEY, name TEXT, dob TEXT, profilePicture TEXT, isVerified BOOLEAN)");
        query.exec("CREATE TABLE IF NOT EXISTS candidates (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT)");
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    AdminMainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}

#include "main.moc"

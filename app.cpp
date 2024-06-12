#include <QApplication>
#include <QMainWindow>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDateEdit>
#include <QTextEdit>

class HomePage : public QWidget {
    Q_OBJECT
public:
    HomePage(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *titleLabel = new QLabel("Cast Your Vote", this);
        titleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(titleLabel);

        candidateComboBox = new QComboBox(this);
        layout->addWidget(candidateComboBox);

        QPushButton *submitVoteButton = new QPushButton("Submit Vote", this);
        connect(submitVoteButton, &QPushButton::clicked, this, &HomePage::handleVote);
        layout->addWidget(submitVoteButton);

        recentVotesLabel = new QLabel("Recent Votes:", this);
        layout->addWidget(recentVotesLabel);

        loadCandidates();
        loadVotes();
    }

public slots:
    void handleVote() {
        QString vote = candidateComboBox->currentText();
        if (vote.isEmpty()) {
            QMessageBox::warning(this, "Error", "You must select a candidate to vote.");
            return;
        }

        QSqlQuery query;
        query.prepare("INSERT INTO votes (vote) VALUES (:vote)");
        query.bindValue(":vote", vote);
        if (!query.exec()) {
            QMessageBox::critical(this, "Error", query.lastError().text());
            return;
        }

        QMessageBox::information(this, "Success", "Vote submitted successfully!");
        loadVotes();
    }

private:
    QComboBox *candidateComboBox;
    QLabel *recentVotesLabel;

    void loadCandidates() {
        QSqlQuery query("SELECT name FROM candidates");
        while (query.next()) {
            candidateComboBox->addItem(query.value(0).toString());
        }
    }

    void loadVotes() {
        QSqlQuery query("SELECT vote FROM votes");
        QStringList votes;
        while (query.next()) {
            votes.append(query.value(0).toString());
        }
        recentVotesLabel->setText("Recent Votes:\n" + votes.join("\n"));
    }
};

class VoteHistoryPage : public QWidget {
    Q_OBJECT
public:
    VoteHistoryPage(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        QLabel *titleLabel = new QLabel("Vote History", this);
        titleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(titleLabel);

        votesTable = new QTableWidget(this);
        votesTable->setColumnCount(1);
        votesTable->setHorizontalHeaderLabels({"Votes"});
        layout->addWidget(votesTable);

        loadVotes();
    }

private:
    QTableWidget *votesTable;

    void loadVotes() {
        QSqlQuery query("SELECT vote FROM votes");
        int row = 0;
        votesTable->setRowCount(query.size());
        while (query.next()) {
            votesTable->setItem(row, 0, new QTableWidgetItem(query.value(0).toString()));
            row++;
        }
    }
};

class ProfilePage : public QWidget {
    Q_OBJECT
public:
    ProfilePage(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);

        QFormLayout *formLayout = new QFormLayout();

        nameEdit = new QLineEdit(this);
        dobEdit = new QDateEdit(this);
        dobEdit->setCalendarPopup(true);
        profilePictureEdit = new QTextEdit(this);

        formLayout->addRow("Full Name:", nameEdit);
        formLayout->addRow("Date of Birth:", dobEdit);
        formLayout->addRow("Profile Picture URL:", profilePictureEdit);

        layout->addLayout(formLayout);

        QPushButton *saveProfileButton = new QPushButton("Save Profile", this);
        connect(saveProfileButton, &QPushButton::clicked, this, &ProfilePage::handleProfileSave);
        layout->addWidget(saveProfileButton);

        loadProfile();
    }

public slots:
    void handleProfileSave() {
        QString name = nameEdit->text();
        QString dob = dobEdit->date().toString("yyyy-MM-dd");
        QString profilePicture = profilePictureEdit->toPlainText();

        QSqlQuery query;
        query.prepare("INSERT OR REPLACE INTO profiles (id, name, dob, profilePicture, isVerified) VALUES (1, :name, :dob, :profilePicture, 0)");
        query.bindValue(":name", name);
        query.bindValue(":dob", dob);
        query.bindValue(":profilePicture", profilePicture);
        if (!query.exec()) {
            QMessageBox::critical(this, "Error", query.lastError().text());
            return;
        }

        QMessageBox::information(this, "Success", "Profile saved successfully!");
    }

private:
    QLineEdit *nameEdit;
    QDateEdit *dobEdit;
    QTextEdit *profilePictureEdit;

    void loadProfile() {
        QSqlQuery query("SELECT name, dob, profilePicture FROM profiles WHERE id = 1");
        if (query.next()) {
            nameEdit->setText(query.value(0).toString());
            dobEdit->setDate(QDate::fromString(query.value(1).toString(), "yyyy-MM-dd"));
            profilePictureEdit->setText(query.value(2).toString());
        }
    }
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        QStackedWidget *stackedWidget = new QStackedWidget(this);

        HomePage *homePage = new HomePage(this);
        VoteHistoryPage *voteHistoryPage = new VoteHistoryPage(this);
        ProfilePage *profilePage = new ProfilePage(this);

        stackedWidget->addWidget(homePage);
        stackedWidget->addWidget(voteHistoryPage);
        stackedWidget->addWidget(profilePage);

        setCentralWidget(stackedWidget);

        QMenuBar *menuBar = new QMenuBar(this);
        QMenu *menu = new QMenu("Menu", this);
        QAction *homeAction = new QAction("Home", this);
        QAction *historyAction = new QAction("Vote History", this);
        QAction *profileAction = new QAction("Profile", this);

        connect(homeAction, &QAction::triggered, [stackedWidget, homePage]() {
            stackedWidget->setCurrentWidget(homePage);
        });

        connect(historyAction, &QAction::triggered, [stackedWidget, voteHistoryPage]() {
            stackedWidget->setCurrentWidget(voteHistoryPage);
        });

        connect(profileAction, &QAction::triggered, [stackedWidget, profilePage]() {
            stackedWidget->setCurrentWidget(profilePage);
        });

        menu->addAction(homeAction);
        menu->addAction(historyAction);
        menu->addAction(profileAction);

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
        query.exec("CREATE TABLE IF NOT EXISTS votes (id INTEGER PRIMARY KEY AUTOINCREMENT, vote TEXT)");
        query.exec("CREATE TABLE IF NOT EXISTS candidates (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT)");

        // Add some default candidates for demonstration
        query.exec("INSERT OR IGNORE INTO candidates (name) VALUES ('Candidate 1')");
        query.exec("INSERT OR IGNORE INTO candidates (name) VALUES ('Candidate 2')");
        query.exec("INSERT OR IGNORE INTO candidates (name) VALUES ('Candidate 3')");
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}

#include "main.moc"

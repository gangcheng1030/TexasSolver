﻿#include "strategyexplorer.h"
#include "ui_strategyexplorer.h"
#include "qstandarditemmodel.h"
#include <QBrush>
#include <QApplication>
#include <QComboBox>
#include <QColor>
#include "include/Card.h"

StrategyExplorer::StrategyExplorer(QWidget *parent,QSolverJob * qSolverJob) :
    QDialog(parent),
    ui(new Ui::StrategyExplorer)
{
    this->qSolverJob = qSolverJob;
    this->detailWindowSetting = DetailWindowSetting();
    ui->setupUi(this);
    /*
    QStandardItemModel* model = new QStandardItemModel();
    for (int row = 0; row < 4; ++row) {
         QStandardItem *item = new QStandardItem(QString("%1").arg(row) );
         model->appendRow( item );
    }
    this->ui->gameTreeView->setModel(model);
    */
    // Initial Game Tree preview panel
    this->ui->gameTreeView->setTreeData(qSolverJob);
    connect(
                this->ui->gameTreeView,
                SIGNAL(expanded(const QModelIndex&)),
                this,
                SLOT(item_expanded(const QModelIndex&))
                );
    connect(
                this->ui->gameTreeView,
                SIGNAL(clicked(const QModelIndex&)),
                this,
                SLOT(item_clicked(const QModelIndex&))
                );

    // Initize strategy(rough) table
    this->tableStrategyModel = new TableStrategyModel(this->qSolverJob,this);
    this->ui->strategyTableView->setModel(this->tableStrategyModel);
    this->delegate_strategy = new StrategyItemDelegate(this->qSolverJob,&(this->detailWindowSetting),this);
    this->ui->strategyTableView->setItemDelegate(this->delegate_strategy);

    Deck* deck = this->qSolverJob->get_solver()->get_deck();
    int index = 0;
    QString board_qstring = QString::fromStdString(this->qSolverJob->board);
    for(Card one_card: deck->getCards()){
        if(board_qstring.contains(QString::fromStdString(one_card.toString())))continue;
        QString card_str_formatted = QString::fromStdString(one_card.toFormattedString());
        this->ui->turnCardBox->addItem(card_str_formatted);
        this->ui->riverCardBox->addItem(card_str_formatted);

        if(card_str_formatted.contains(QString::fromLocal8Bit("♦️")) ||
                card_str_formatted.contains(QString::fromLocal8Bit("♥️️"))){
            this->ui->turnCardBox->setItemData(0, QBrush(Qt::red),Qt::ForegroundRole);
            this->ui->riverCardBox->setItemData(0, QBrush(Qt::red),Qt::ForegroundRole);
        }else{
            this->ui->turnCardBox->setItemData(0, QBrush(Qt::black),Qt::ForegroundRole);
            this->ui->riverCardBox->setItemData(0, QBrush(Qt::black),Qt::ForegroundRole);
        }

        this->cards.push_back(one_card);
        index += 1;
    }
    if(this->qSolverJob->get_solver()->getGameTree()->getRoot()->getRound() == GameTreeNode::GameRound::FLOP){
        this->tableStrategyModel->setTrunCard(this->cards[0]);
        this->tableStrategyModel->setRiverCard(this->cards[1]);
        this->ui->riverCardBox->setCurrentIndex(1);
    }
    else if(this->qSolverJob->get_solver()->getGameTree()->getRoot()->getRound() == GameTreeNode::GameRound::TURN){
        this->tableStrategyModel->setRiverCard(this->cards[0]);
        this->ui->turnCardBox->clear();
    }
    else if(this->qSolverJob->get_solver()->getGameTree()->getRoot()->getRound() == GameTreeNode::GameRound::RIVER){
        this->ui->turnCardBox->clear();
        this->ui->riverCardBox->clear();
    }

    // Initize timer for strategy auto update
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update_second()));
    timer->start(1000);

    // On mouse event of strategy table
    connect(this->ui->strategyTableView,SIGNAL(itemMouseChange(int,int)),this,SLOT(onMouseMoveEvent(int,int)));

    // Initize Detail Viewer window
    this->detailViewerModel = new DetailViewerModel(this->tableStrategyModel,this);
    this->ui->detailView->setModel(this->detailViewerModel);
    this->detailItemItemDelegate = new DetailItemDelegate(&(this->detailWindowSetting),this);
    this->ui->detailView->setItemDelegate(this->detailItemItemDelegate);
}

StrategyExplorer::~StrategyExplorer()
{
    delete ui;
    delete this->delegate_strategy;
    delete this->tableStrategyModel;
    delete this->detailViewerModel;
    delete this->timer;
}

void StrategyExplorer::item_expanded(const QModelIndex& index){
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    int num_child = item->childCount();
    for (int i = 0;i < num_child;i ++){
        TreeItem* one_child = item->child(i);
        if(one_child->childCount() != 0)continue;
        this->ui->gameTreeView->tree_model->reGenerateTreeItem(one_child->m_treedata.lock()->getRound(),one_child);
    }
}

void StrategyExplorer::item_clicked(const QModelIndex& index){
    try{
        TreeItem * treeNode = static_cast<TreeItem*>(index.internalPointer());
        this->tableStrategyModel->setGameTreeNode(treeNode);
        this->tableStrategyModel->updateStrategyData();
        this->ui->strategyTableView->viewport()->update();
    }
    catch (const runtime_error& error)
    {
        qDebug().noquote() << tr("Encountering error:");//.toStdString() << endl;
        qDebug().noquote() << error.what() << "\n";
    }
}

void StrategyExplorer::selection_changed(const QItemSelection &selected,
                                         const QItemSelection &deselected){
}

void StrategyExplorer::on_turnCardBox_currentIndexChanged(int index)
{
    if(this->cards.size() > 0 && index < this->cards.size()){
        this->tableStrategyModel->setTrunCard(this->cards[index]);
        this->tableStrategyModel->updateStrategyData();
    }
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
}

void StrategyExplorer::on_riverCardBox_currentIndexChanged(int index)
{
    if(this->cards.size() > 0  && index < this->cards.size()){
        this->tableStrategyModel->setRiverCard(this->cards[index]);
        this->tableStrategyModel->updateStrategyData();
    }
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
}

void StrategyExplorer::update_second(){
    if(this->cards.size() > 0){
        this->tableStrategyModel->updateStrategyData();
    }
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
}


void StrategyExplorer::onMouseMoveEvent(int i,int j){
    this->detailWindowSetting.grid_i = i;
    this->detailWindowSetting.grid_j = j;
    this->ui->detailView->viewport()->update();
}

void StrategyExplorer::on_strategyModeButtom_clicked()
{
    this->detailWindowSetting.mode = DetailWindowSetting::DetailWindowMode::STRATEGY;
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
}

void StrategyExplorer::on_ipRangeButtom_clicked()
{
    this->detailWindowSetting.mode = DetailWindowSetting::DetailWindowMode::RANGE_IP;
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
}

void StrategyExplorer::on_oopRangeButtom_clicked()
{
    this->detailWindowSetting.mode = DetailWindowSetting::DetailWindowMode::RANGE_OOP;
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
}

void StrategyExplorer::on_evModeButtom_clicked()
{
    this->detailWindowSetting.mode = DetailWindowSetting::DetailWindowMode::EV;
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
}

void StrategyExplorer::on_evOnlyModeButtom_clicked()
{
    this->detailWindowSetting.mode = DetailWindowSetting::DetailWindowMode::EV_ONLY;
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
}

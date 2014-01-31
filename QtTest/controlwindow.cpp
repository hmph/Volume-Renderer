/*
*	controlwindow.cpp
*	Anurag Arnab
*	23 January 2013
*
*	Implemenation of Control Window - the "Controller" in the Model-View-Control (MVC) design pattern
*/

#include "controlwindow.h"

/*
*
* Constructor. Creates all the widgets and adds them to a vertical layout
* All widgets are created programatically
* The widgets for the "render" submenu are created in a separate function - initialiseRenderOptions()
*
*/
ControlWindow::ControlWindow(QWidget *parent)
: QWidget(parent)
{
	uploadOCT = new QCommandLinkButton("Upload OCT");
	renderFilter = new QCommandLinkButton("Render Filter");
	volumeRendering = new QCommandLinkButton("Volume Rendering");
	computeFingerprint = new QCommandLinkButton("Compute 2D-Fingerprint");
	enhanceFingerprint = new QCommandLinkButton("Enhance Fingerprint");

	renderLayout.addWidget(renderFilter);

	mainLayout.addWidget(uploadOCT);
	mainLayout.addLayout(&renderLayout);
	mainLayout.addWidget(volumeRendering);
	mainLayout.addWidget(computeFingerprint);
	mainLayout.addWidget(enhanceFingerprint);

	connect(volumeRendering, SIGNAL(clicked()), this, SLOT(handleVolumeRendering()));
	connect(renderFilter, SIGNAL(clicked()), this, SLOT(handleRenderFilter()));
	connect(uploadOCT, SIGNAL(clicked()), this, SLOT(handleUploadOCT()));
	connect(computeFingerprint, SIGNAL(clicked()), this, SLOT(handleComputeFingerprint()));
	connect(enhanceFingerprint, SIGNAL(clicked()), this, SLOT(handleEnhanceFingerprint()));

	radioButtons = new QRadioButton *[N_FILTERS];
	filterOptions = new QLineEdit *[N_FILTERS];

	initialiseRenderOptions();

	setLayout(&mainLayout);
	setWindowTitle("Control Window");
}

ControlWindow::~ControlWindow()
{

}

/*
*
* Slot for the volume rendering button
* Gets the location of the config file for the volume renderer by prompting a file dialog
*
*/
void ControlWindow::handleVolumeRendering(void)
{
	qDebug() << "Volume Rendering slot called";
	QString fileName = QFileDialog::getOpenFileName(this, "Specify the config file for the volume renderer", QDir::currentPath());
	emit volRendConfigFile(fileName);

}

/*
*
* Slot for handling the render button press
* Shows or hides the rendering submenu depending on the click
*
*/
void ControlWindow::handleRenderFilter(void)
{

	static int count = 0;

	if (count % 2 == 1)
	{
		++count;
		groupRenderFilter->hide();
		return;
	}

	qDebug() << "Render filter slot called";

	groupRenderFilter->show();
	++count;
}


/*
*
* Slot for OCT button
* Prompts user to select first image in sequence
* Images must have a number at the end
*
*/
void ControlWindow::handleUploadOCT(void)
{
	qDebug() << "Upload OCT slot called";
	QString fileName = QFileDialog::getOpenFileName(this, "Specify first image file in sequence", QDir::currentPath());
	emit imageFilename(fileName);
}

/*
*
* Slot for handling the fingeprint button
* Does nothing at the moment
*
*/
void ControlWindow::handleComputeFingerprint(void)
{
	qDebug() << "Compute fingerprint slot called";

}

/*
*
* Another unimplemented slot
*
*/
void ControlWindow::handleEnhanceFingerprint(void)
{
	qDebug() << "Enhance fingerprint slot called";
}

/*
*
* Creates the rendering submenu
*
*/
void ControlWindow::initialiseRenderOptions(void)
{
	groupRenderFilter = new QGroupBox("Filters");
	QVBoxLayout *vRadioBox = new QVBoxLayout();

	for (int i = 0; i < N_FILTERS; ++i){
		radioButtons[i] = new QRadioButton(QString("Filter(%1)").arg(i + 1));
		filterOptions[i] = new QLineEdit("Parameters");

		filterOptions[i]->hide();

		connect(radioButtons[i], SIGNAL(toggled(bool)), this, SLOT(radioButtonToggled()));
		connect(filterOptions[i], SIGNAL(textChanged(QString)), this, SLOT(radioButtonToggled()));

		vRadioBox->addWidget(radioButtons[i]);
		vRadioBox->addWidget(filterOptions[i]);
	}

	groupRenderFilter->setLayout(vRadioBox);
	renderLayout.addWidget(groupRenderFilter);

	groupRenderFilter->hide();
}

/*
* Called whenever a radio button is toggled
* Since the radio buttons are mutually exclusive, this function is called twice each time a radio button is pressed
* Once for the radio button being de-selected and once for the button being selected *

* This slot is also called when the text is changed
* */
void ControlWindow::radioButtonToggled(void)
{
	for (int i = 0; i < N_FILTERS; ++i)
	{
		if (radioButtons[i]->isChecked()){
			filterOptions[i]->show();

			QString text = filterOptions[i]->text();
			bool ok;

			int parameter = text.toInt(&ok);
			if (ok){
				emit imageFilterChosen(i, parameter);
			}

		}
		else{
			filterOptions[i]->hide();
		}
	}
}

/*
*
* Event handler for closing the window
* Emits a signal to force the other window to close as well
*
*/
void ControlWindow::closeEvent(QCloseEvent *event)
{
	emit closeWindow();
	event->accept();
}

/*
*
* Closes the window. This slot is signalled when the rendering window closes
*
*/
void ControlWindow::forceClose(void)
{
	close();
}
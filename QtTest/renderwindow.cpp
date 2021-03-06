/*
*	renderwindow.cpp
*	Anurag Arnab
*	23 January 2014
*
*	Window which is the "View" in the Model-View-Controller design pattern
*   Shows the volume rendering as well as the images
*/

#include "renderwindow.h"
#include <QDebug>
#include "busywindow.h"

#include "glWidget.h"

/*
*
* Constructor.
* A QSplitter is used to allow resizing of the Volume Renderer, and 2 Image views
* Function pointers are also set up to the filtering functions
*
*/
RenderWindow::RenderWindow(int numberFilters, QWidget *parent) :
QWidget(parent)
{
	// setting image reading settings
	currentNo = 0;
	minNo = 0;
	maxNo = 0;
	paddingLength = 0;
	volumeRenderer = NULL;
	processOption = -1;

	scene = new QGraphicsScene();
	scene2 = new QGraphicsScene();
	imageSelector = new QSlider(Qt::Horizontal);
	imageView = new QGraphicsView(scene);
	imageView2 = new QGraphicsView(scene2);
	imageNo = new QLabel();
	
	scene->addText("Loaded image");
	scene2->addText("Filtered image");

	sliderSplit = new QSplitter();
	imageSplit = new QSplitter();
	mainSplit = new QSplitter();

	sliderLayout.addWidget(imageSelector);
	sliderLayout.addWidget(imageNo);

	imageSplit->addWidget(imageView);
	imageSplit->addWidget(imageView2);

	mainLayout.addLayout(&sliderLayout);
	mainSplit->addWidget(imageSplit);
	mainSplit->setOrientation(Qt::Vertical);

	mainLayout.addWidget(mainSplit);

	processingFunctions = new ProcessFn[numberFilters];
	processingFunctions[0] = &RenderWindow::processGrayscale;
	processingFunctions[1] = &RenderWindow::processBrightness;
	processingFunctions[2] = &RenderWindow::processBlur;
	processingFunctions[3] = &RenderWindow::processSaturation;
	processingFunctions[4] = &RenderWindow::processStub;

	setLayout(&mainLayout);
	setWindowTitle("Rendering window");
	setFixedSize(WINDOW_WIDTH, WINDOW_HEIGHT-VOL_REND_HEIGHT);
}


RenderWindow::~RenderWindow()
{
}

/*
*
* Loads an image into the first Graphics View
* Loads a filtered version of ths image if the option is enabled
* The variable "currentNo" stores the current image to be displayed
*
*/
bool RenderWindow::loadImages(void)
{
	QImage image;
	image.load(getFilename(currentNo));

	if (image.isNull()){
		qDebug() << "Could not open " << getFilename(currentNo);
		return false;
	}

	/*scene->clear();
	QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
	scene->addItem(item);*/

	assignImage(scene, &image);

	if (processOption > -1){
		(this->*processingFunctions[processOption])(processParameter, &image);
	}

	return true;
}

/*
*
* Receives filename of first image in sequence
* It then determines the total number of images in the sequence
* To do this, the prefix of the image is first extracted so that this can be prepended to the image number
* The extension of the image is also determined 
*
*/
void RenderWindow::initialiseImages(QString filename)
{

	QString digits;
	QString temp = filename;

	int extensionIndex = temp.lastIndexOf(".");
	if (extensionIndex >= 0)
	{
		extension = temp.remove(0, extensionIndex + 1).trimmed();
	}
	qDebug() << filename;

	QRegularExpression re("(.*?)([\\d]+)\\..*");
	QRegularExpressionMatch match = re.match(filename);
	if (match.hasMatch()) {
		filePrefix = match.captured(1); // "image
		digits = match.captured(2); // "001
		qDebug() << digits;
		qDebug() << filePrefix;
		qDebug() << extension;

		minNo = digits.toInt();
		currentNo = minNo;
		maxNo = minNo;
		paddingLength = digits.length();

		qDebug() << getFilename(currentNo);

		computeTotalImages();
		initialiseSlider();
	}

	if (volumeRenderer != NULL){
		volumeRenderer->displayBlack();
	}

}

/*
*
* Generates the filename, padding the number with as many places as the first image selected
* For example, prefix = "test", number = 8 ----> test008 (if the first image was test001 or test002 etc)
*
*/
QString RenderWindow::getFilename(int number)
{
	QString result = "";
	for (int i = 0; i < paddingLength - 1; ++i){
		result.append("0");
	}
	result = result + QString::number(number);
	return filePrefix + result.remove(0, result.length() - paddingLength) + "." + extension;
}

/*
*
* Determines the total number of images in the sequence
* This is done by incrementing the number and seeing if the file exists
* The minimum number is the number of the fist image read
*
*/
void RenderWindow::computeTotalImages(void)
{
	maxNo = minNo;
	QFile test(getFilename(minNo));

	while (test.exists()){
		test.close();
		test.setFileName(getFilename(++maxNo));
	}

	--maxNo;
	qDebug() << "Max number " << maxNo;
}

/*
*
* Initialise the slider
* Connect its signals and slots
*
*/
void RenderWindow::initialiseSlider(void)
{
	imageSelector->setMinimum(minNo);
	imageSelector->setMaximum(maxNo);

	currentNo = minNo;
	imageSelector->setValue(currentNo);

	connect(imageSelector, SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)));
	emit imageSelector->valueChanged(imageSelector->value());
	//sliderChanged(0);// Above line creates a problem in Qt 4.8
}

/*
*
* Slot. Called when the slider is changed
* Updates the text box and updates the image being displayed
*
*/
void RenderWindow::sliderChanged(int newNumber)
{
	currentNo = newNumber;
	if (loadImages()){
		imageNo->setText(QString::number(currentNo) + "/" + QString::number(maxNo));
	}
}

/*
*
* Add a volume rendering widget to the window
*
*/
void RenderWindow::addVolumeRenderer(glWidget * volumeRenderer)
{
	mainLayout.addWidget(volumeRenderer);
}

/*
*
* Create a new volume rendering widget and add it to the window
*
*/
void RenderWindow::initVolRenderer(QString filename)
{

	BusyWindow b;
	b.show();

	QCoreApplication::processEvents();

	if (volumeRenderer == NULL){
		volumeRenderer = /*new glWidget(filename)*/ new glWidget(filePrefix, extension, paddingLength, minNo, maxNo);
		volumeRenderer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		volumeRenderer->setFixedHeight(VOL_REND_HEIGHT);
		mainSplit->addWidget(volumeRenderer);
		setFixedSize(WINDOW_WIDTH, WINDOW_HEIGHT);
		loadImages();

	}
	else{
		//volumeRenderer->loadNewFile(filename);
		volumeRenderer->loadNewFile(filePrefix, extension, paddingLength, minNo, maxNo);
	}
}

void RenderWindow::initVolRendererThread(QString filename)
{

	if (volumeRenderer == NULL){

		volumeRenderer = /*new glWidget(filename)*/ new glWidget(filePrefix, extension, paddingLength, minNo, maxNo);
		mainSplit->addWidget(volumeRenderer);
	}
	else{
		volumeRenderer->loadNewFile(filePrefix, extension, paddingLength, minNo, maxNo);
	}
}


/*
*
* Slot. 
* Gets the filtering option from the other window
*
*/
void RenderWindow::getProcessOption(int number, QString parameter)
{
	processOption = number;
	processParameter = parameter;
	loadImages();
}

/*
*
* Grayscale filter
*
* Doesn't use any parameters but needed because all functions in function pointer array have the same signature
*/
void RenderWindow::processGrayscale(QString parameters, QImage * image)
{

	QRgb * line;

	for (int y = 0; y<image->height(); ++y){
		QRgb * line = (QRgb *)image->scanLine(y);

		for (int x = 0; x<image->width(); ++x){
			int average = (qRed(line[x]) + qGreen(line[x]) + qRed(line[x])) / 3;
			image->setPixel(x, y, qRgb(average, average, average));
		}
	}

	assignImage(scene2, image);
}

/*
*
* Brightness/Darkness filter
* Adds the same value, delta, to the RGB channels
* Positive value makes image brighter, negative value darkens
*
*/
void RenderWindow::processBrightness(QString parameters, QImage * image)
{
	bool ok = false;
	int delta = parameters.toInt(&ok);

	if (!ok)
	{
		return;
	}

	
	QColor oldColor;
	int r, g, b;

	for (int x = 0; x<image->width(); ++x){
		for (int y = 0; y<image->height(); ++y){

			oldColor = QColor(image->pixel(x, y));

			r = oldColor.red() + delta;
			g = oldColor.green() + delta;
			b = oldColor.blue() + delta;

			//check if the new values are between 0 and 255
			r = qBound(0, r, 255);
			g = qBound(0, g, 255);
			b = qBound(0, b, 255);

			image->setPixel(x, y, qRgb(r, g, b));
		}
	}

	assignImage(scene2, image);
}

/*
*
* Blur filter
* Uses a 5x5 matrix as its kernel
* The parameter kernel determines the values of the matrix
*
*/
void RenderWindow::processBlur(QString parameters, QImage * image)
{

	bool ok = false;
	int kernel = parameters.toInt(&ok);

	if (!ok)
	{
		return;
	}

	if (kernel == 0)
	{
		assignImage(scene2, image);
		return;
	}

	int matrix[5][5] = 
	{ { 0, 0, kernel/4, 0, 0 },
	{ 0, kernel/4, kernel/2, kernel/4, 0 },
	{ kernel/4, kernel/2, kernel, kernel/2, kernel/4 },
	{ 0, kernel/4, kernel/2, kernel/4, 0 },
	{ 0, 0, kernel/4 , 0, 0 } };
	
	int kernelSize = 5;
	
	int sumKernel = 0;
	for (int row = 0; row < 5; ++row){
		for (int col = 0; col < 5; ++col){
			sumKernel += matrix[row][col];
		}
	}
	
	int r, g, b;
	QColor color;

	for (int x = kernelSize / 2; x< image->width() - (kernelSize / 2); x++){
		for (int y = kernelSize / 2; y< image->height() - (kernelSize / 2); y++){

			r = 0;
			g = 0;
			b = 0;

			for (int i = -kernelSize / 2; i <= kernelSize / 2; i++){
				for (int j = -kernelSize / 2; j <= kernelSize / 2; j++){
					color = QColor(image->pixel(x + i, y + j));
					r += color.red()*matrix[kernelSize / 2 + i][kernelSize / 2 + j];
					g += color.green()*matrix[kernelSize / 2 + i][kernelSize / 2 + j];
					b += color.blue()*matrix[kernelSize / 2 + i][kernelSize / 2 + j];
				}
			}

			r = qBound(0, r / sumKernel, 255);
			g = qBound(0, g / sumKernel, 255);
			b = qBound(0, b / sumKernel, 255);

			image->setPixel(x, y, qRgb(r, g, b));

		}
	}

	assignImage(scene2, image);
}

/*
*
* Saturation filter
* Increases or decreases the saturation
*
*/
void RenderWindow::processSaturation(QString parameters, QImage * image)
{
	bool ok = false;
	int delta = parameters.toInt(&ok);

	if (!ok)
	{
		return;
	}

	
	QColor oldColor;
	QColor newColor;
	int h, s, l;

	for (int x = 0; x< image->width(); x++){
		for (int y = 0; y<image->height(); y++){
			oldColor = QColor(image->pixel(x, y));

			newColor = oldColor.toHsl();
			h = newColor.hue();
			s = newColor.saturation() + delta;
			l = newColor.lightness();

			//check if the new value is between 0 and 255
			s = qBound(0, s, 255);

			newColor.setHsl(h, s, l);

			image->setPixel(x, y, qRgb(newColor.red(), newColor.green(), newColor.blue()));
		}
	}

	assignImage(scene2, image);
}

void RenderWindow::processStub(QString parameters, QImage * image)
{
	qDebug() << "Unimplemented filter. Received parameters: " << parameters;
}

/*
*
* Loads an image into the specifed graphics scene
*
*/
void RenderWindow::assignImage(QGraphicsScene * gScene, QImage * gImage)
{
	
	QImage resized = gImage->scaled(imageView->width(), imageView->height()-3, Qt::AspectRatioMode::KeepAspectRatio);

	//qDebug() << resized.width() << " " << resized.height();
	
	gScene->clear();
	QGraphicsPixmapItem* item2 = new QGraphicsPixmapItem(QPixmap::fromImage(resized));
	gScene->addItem(item2);

	//Calculates and returns the bounding rect of all items on the scene.
	//This function works by iterating over all items, and because if this, it can be slow for large scenes.
	QRectF rect = gScene->itemsBoundingRect();
	gScene->setSceneRect(rect);
}

/*
*
* Closes the window. This slot is signalled when the rendering window closes
*
*/
void RenderWindow::forceClose(void)
{
	close();
}

/*
*
* Event handler for closing the window
* Emits a signal to force the other window to close as well
*
*/
void RenderWindow::closeEvent(QCloseEvent *event)
{
	emit closeWindow();
	event->accept();
}


/*
*
* Slot which receives the alpha threshold and conveys this to the volume renderer
*
*/
void RenderWindow::getAlphaThresh(QString line)
{
	float value;
	bool isSuccess = false;

	value = line.toFloat(&isSuccess);

	if (isSuccess && value >= 0.0f && value <= 1.0f && volumeRenderer != NULL){
		volumeRenderer->setAlphaThresh(value);
	}
}


/*
*
* Slot which receives the alpha scaling factor and conveys this to the volume renderer
* "QCoreApplication::processEvents()" is used to allow the Busy Window to be rendered before the volume renderer is changed
* The reason for this is that Qt updates the screen asynchronously.
* So if this is not done first, the Busy Window will not finish rendering completely
*
*/
void RenderWindow::getAlphaScale(QString line)
{

	bool isSuccess = false;
	float value = line.toFloat(&isSuccess);

	if (isSuccess && value >= 0.0f && value <= 1.0f && volumeRenderer != NULL){

		BusyWindow b;
		b.show();
		QCoreApplication::processEvents();

		volumeRenderer->setAlphaScale(value);
	}
}
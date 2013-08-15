
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __PLUGINPROCESSOR__
#define __PLUGINPROCESSOR__

#include "MLDSPEngine.h"
#include "MLAudioProcessorListener.h"
#include "MLDefaultFileLocations.h"
#include "MLModel.h"
#include "MLPluginEditor.h"
#include "MLPluginFormats.h"

#include <vector>
#include <map>

const int kMLPatcherMaxTableSize = 64;

// MLPluginProcessor wraps up an AudioProcessor in an MLModel interface. 
// The audio wrapper code needs to get at AudioProcessor directly, so this
// is a composition of MLModel with AudioProcessor rather than a wrapper, 
// which would otherwise be cleaner. 
//

class MLPluginProcessor : 
	public AudioProcessor,
	public MLModel
{
public:
		
	MLPluginProcessor();
    ~MLPluginProcessor();
	void setWrapperFormat(int format);
	
	// --------------------------------------------------------------------------------
	// plugin description and default preset
	void loadPluginDescription(const char* desc);
	virtual void loadDefaultPreset() = 0;
	
	// initializeProcessor is called after graph is created.
	virtual void initializeProcessor() = 0; 

	// --------------------------------------------------------------------------------
	// preflight and cleanup
	MLProc::err preflight();
	virtual bool wantsMIDI() {return true;}
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();
	void setDefaultParameters();
	void reset();

	// --------------------------------------------------------------------------------
	// process
	bool isOKToProcess();
	void processMIDI (MidiBuffer& midiMessages);
	void setCollectStats(bool k);
    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

	// --------------------------------------------------------------------------------
	bool hasEditor() const { return true; }
	AudioProcessorEditor* createEditor();
    const String getName() const        { return JucePlugin_Name; }

	// --------------------------------------------------------------------------------
	// parameters
    int getNumParameters();
	int getParameterIndex (const MLSymbol name);
	float getParameter (int index);
    void setParameter (int index, float newValue);
	void setParameter (MLSymbol paramName, float newValue);
	float getParameterAsLinearProportion (int index);
	void setParameterAsLinearProportion (int index, float newValue);
	void MLSetParameterNotifyingHost (const int parameterIndex, const float newValue);

    const String getParameterName (int index);
	const String symbolToXMLAttr(const MLSymbol sym);
	const MLSymbol XMLAttrToSymbol(const String& str);
	
    const MLSymbol getParameterAlias (int index);
    const MLParamValue getParameterDefault (int index);
    const String getParameterText (int index);
	float getParameterMin (int index);
	float getParameterMax (int index);
	
	MLPublishedParamPtr getParameterPtr (int index);
	MLPublishedParamPtr getParameterPtr (MLSymbol sym);
	const std::string& getParameterGroupName (int index);
		
	// --------------------------------------------------------------------------------
	// signals
	int countSignals(const MLSymbol alias);
	unsigned readSignal(const MLSymbol alias, MLSignal& outSig);

	// temp
	MLProcList& getPatcherList();

	// --------------------------------------------------------------------------------
	// state
	virtual void getStateAsXML (XmlElement& xml);
	virtual void setStateFromXML(const XmlElement& xmlState);
	int saveStateAsVersion(const File& f);
	int saveStateOverPrevious(const File& f);
	void returnToLatestStateLoaded();
	
	void getStateAsText (String& destStr);
	void setStateFromText (const String& stateStr);

	void getStateInformation (juce::MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);

	void saveStateToFile(File& saveFile, int format);
	void loadStateFromFile(const File& loadFile);

	// --------------------------------------------------------------------------------
	// MIDI programs
	
	void clearMIDIProgramFiles();
	void setMIDIProgramFile(int pgm, File f);
	void setStateFromMIDIProgram (const int pgmIdx);
	void scanMIDIPrograms();

	// --------------------------------------------------------------------------------
	// channels
	
    const String getInputChannelName (const int channelIndex) const;
    const String getOutputChannelName (const int channelIndex) const;
    bool isInputChannelStereoPair (int index) const;
    bool isOutputChannelStereoPair (int index) const;

	double getTailLengthSeconds() const;
	bool acceptsMidi() const;
    bool producesMidi() const;
	
	// --------------------------------------------------------------------------------
	// factory presets - a VST concept - unimplemented
	// 
    int getNumPrograms()                                        { return 0; }
    int getCurrentProgram()                                     { return 0; }
    void setCurrentProgram (int)                          { }
    const String getProgramName (int)                     { return String::empty; }
    void changeProgramName (int, const String&)   { }

	// --------------------------------------------------------------------------------
	// presets
	
	const String getExtensionForFormat(int format);
	const String& getCurrentPresetName();
	const String& getCurrentPresetDir();
	const String& getCurrentScaleName();

	// used by wrapper
	void setCurrentPresetName(const char* name);
	
	// --------------------------------------------------------------------------------
    AudioPlayHead::CurrentPositionInfo lastPosInfo;

	// --------------------------------------------------------------------------------

	void setMLListener (MLAudioProcessorListener* const newListener) throw();
    MLProc::err sendMessageToMLListener (unsigned msg, const File& f);

	// --------------------------------------------------------------------------------
	// scales
	
	void loadScale(const File& f);
	void loadDefaultScale();
	void setScalesFolder(const File f);
	virtual void broadcastScale(const MLScale* pScale) = 0;
	
	MLDSPEngine* getEngine() { return &mEngine; }
	
	// --------------------------------------------------------------------------------
	// debug
	
	inline void showEngine() { mEngine.dump(); }
	
	// errors TODO do this reusably
	
	const String& getErrorMessage() { return mErrorMessage; }
	void setErrorMessage(const String& s) { mErrorMessage = s; }
	
protected:
	// Engine creates graphs of Processors, does the work
	MLDSPEngine	mEngine;	
	
	typedef std::tr1::shared_ptr<XmlElement> XmlElementPtr;
	XmlElementPtr mpLatestStateLoaded;

	// TODO shared_ptr
	ScopedPointer<XmlDocument> mpPluginDoc;
	String mDocLocationString;
	
	// patcher and other UI element ptrs TEMP
	// MLMatrixConnectionList mMatrixData;
		
private:

	void setCurrentPresetDir(const char* name);
	void setCurrentScaleName(const char* name);
	
	MLAudioProcessorListener* MLListener;

	// number of parameters stored here so we can access it before engine compile
	int mNumParameters; 
	
	// True when any parameters have been set by the host. 
	// If the host doesn't give us a program to load, we can use this to 
	// decide to load defaults after compile.
	bool mHasParametersSet;

	// temp storage for parameter data given to us before our DSP graph is made.
	juce::MemoryBlock mSavedParamBlob;	
		
	// set the plugin state from a memory blob containing parameter and patcher settings.
	void setStateFromBlob (const void* data, int sizeInBytes);
	
	String mCurrentPresetName;
	String mCurrentPresetDir;
	String mCurrentScaleName;
	String mCurrentScaleDir;

	File mFactoryPresetsFolder, mUserPresetsFolder, mScalesFolder;
	bool mFileLocationsOK;
		
	std::vector<File> mMIDIProgramFiles;
	
	int mWrapperFormat;
	
	// state for editor
	MLRect mEditorRect;
	bool mEditorNumbersOn;
	bool mEditorAnimationsOn;
	
	bool mInitialized;
	
	String mErrorMessage;
};

#endif  // __PLUGINPROCESSOR__
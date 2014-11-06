
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLAppState.h"

MLAppState::MLAppState(MLModel* pM, const char* makerName, const char* appName, int version) :
    MLPropertyListener(pM),
	mpModel(pM),
	mpMakerName(makerName),
	mpAppName(appName),
	mAppVersion(version)
{
	updateAllProperties();
	startTimer(1000);
}

MLAppState::~MLAppState()
{

}

void MLAppState::timerCallback()
{
	updateChangedProperties();
}

// MLPropertyListener implementation
// an updateChangedProperties() is needed to get these actions sent by the Model.
// 
void MLAppState::doPropertyChangeAction(MLSymbol p, const MLProperty & val)
{
    // nothing to do here, but we do need to be an MLPropertyListener in order to
    // know the update states of all the Properties.
	//debug() << "MLAppState::doPropertyChangeAction: " << p << " to " << val << "\n";
}

#pragma mark get and save state

MemoryBlock MLAppState::getStateAsBinary()
{
	MemoryBlock bIn;
	String stateStr = getStateAsText();
	debug() << "STATE: \n" << stateStr << "\n";
	
	if(stateStr.length() > 0)
	{
		int cLen = CharPointer_UTF8::getBytesRequiredFor (stateStr.toUTF8());
		bIn.replaceWith(stateStr.toUTF8(), cLen);
		// TODO compress here
	}
	return bIn;
}

String MLAppState::getStateAsText()
{
	String r;
	cJSON* root = cJSON_CreateObject();
	if(root)
	{
		getStateAsJSON(root);
		r = cJSON_Print(root);
	}
	else
	{
		debug() << "MLAppState::getStateAsText: couldn't create JSON object!\n";
	}
	return r;
}

void MLAppState::saveStateToStateFile()
{
	// get directory
	File dir(getAppStateDir());
	if (dir.exists())
	{
		if(!dir.isDirectory())
		{
			MLError() << "MLAppState:: file present instead of directory " << 
				dir.getFileName() << "!  Aborting.\n";
		}
	}
	else 
	{
		// make directory		
		Result r = dir.createDirectory();
		if(r.failed())
		{
			MLError() << "Error creating state directory: " << r.getErrorMessage() << "\n";
			return;
		}
	}
	
	// make state file
	File stateFile(getAppStateFile());
	if (!stateFile.exists())
	{
		Result r = stateFile.create();
		if(r.failed())
		{
			MLError() << "Error creating state file: " << r.getErrorMessage() << "\n";
			return;
		}
	}
	
	// get app state as JSON container
	cJSON* root = cJSON_CreateObject();
	if(root)
	{
		getStateAsJSON(root);
	}
	else		
	{
		debug() << "MLAppState::saveStateToStateFile: couldn't create JSON object!\n";
	}
	String stateStr(cJSON_Print(root));
	stateFile.replaceWithText(stateStr);
	if(root)
	{
		cJSON_Delete(root);
	}
}

void MLAppState::getStateAsJSON(cJSON* root)
{
	updateAllProperties();

	// get Model parameters
	std::map<MLSymbol, PropertyState>::iterator it;
	for(it = mPropertyStates.begin(); it != mPropertyStates.end(); it++)
	{
		MLSymbol key = it->first;
		const char* keyStr = key.getString().c_str();
		PropertyState& state = it->second;
		switch(state.mValue.getType())
		{
			case MLProperty::kFloatProperty:
				cJSON_AddNumberToObject(root, keyStr, state.mValue.getFloatValue());
				break;
			case MLProperty::kStringProperty:
				cJSON_AddStringToObject(root, keyStr, state.mValue.getStringValue().c_str());
				break;
			case MLProperty::kSignalProperty:
			{
				// make and populate JSON object representing signal
				cJSON* signalObj = cJSON_CreateObject();
				const MLSignal& sig = state.mValue.getSignalValue();
				cJSON_AddStringToObject(signalObj, "type", "signal");
				cJSON_AddNumberToObject(signalObj, "width", sig.getWidth());
				cJSON_AddNumberToObject(signalObj, "height", sig.getHeight());
				cJSON_AddNumberToObject(signalObj, "depth", sig.getDepth());
				int size = sig.getSize();
				float* pSignalData = sig.getBuffer();
				cJSON* data = cJSON_CreateFloatArray(pSignalData, size);
				cJSON_AddItemToObject(signalObj, "data", data);
				
				// add signal object to state JSON
				cJSON_AddItemToObject(root, keyStr, signalObj);
			}
				break;
			default:
				MLError() << "MLAppState::saveStateToStateFile(): undefined param type! \n";
				break;
		}
	}
	
	// replace environment info
	cJSON_ReplaceItemInObject(root, "maker_name", cJSON_CreateString(mpMakerName));
	cJSON_ReplaceItemInObject(root, "app_name", cJSON_CreateString(mpAppName));
	cJSON_ReplaceItemInObject(root, "app_version", cJSON_CreateNumber(mAppVersion));
}

#pragma mark load and set state

void MLAppState::setStateFromBinary(const MemoryBlock& bIn)
{
	const void* inData = bIn.getData();
	unsigned int inSize = bIn.getSize();
	// TODO uncompress here
	String stateStr = juce::String::fromUTF8(static_cast<const char *>(inData), inSize);

	debug() << "BINARY INPUT STATE: \n" << stateStr << "\n";

	setStateFromText(stateStr);
}

bool MLAppState::loadStateFromAppStateFile()
{
	bool r = false;
	File stateFile = getAppStateFile();
	if(stateFile.exists())
	{
		r = setStateFromText(stateFile.loadFileAsString());
	}
	else
	{
		debug() << "MLAppState::loadStateFromAppStateFile: couldn't open file!\n";
	}
	return r;
}

bool MLAppState::setStateFromText(String stateStr)
{
	bool r = false;
	cJSON* root = cJSON_Parse(stateStr.toUTF8());
	if(root)
	{
		setStateFromJSON(root);
		cJSON_Delete(root);
		r = true;
	}
	else
	{
		debug() << "MLAppState::setStateFromText: couldn't create JSON object!\n";
#ifdef ML_DEBUG
		debug() << "STATE:\n" << 	stateStr << "\n";
#endif
	}
	return r;
}

void MLAppState::setStateFromJSON(cJSON* pNode, int depth)
{
	while(pNode)
	{
		if(pNode->string)
		{
			switch(pNode->type)
			{
				case cJSON_Number:
					//debug() << " depth " << depth << " loading float param " << pNode->string << " : " << pNode->valuedouble << "\n";
					mpModel->setProperty(MLSymbol(pNode->string), (float)pNode->valuedouble);
					break;
				case cJSON_String:
					//debug() << " depth " << depth << " loading string param " << pNode->string << " : " << pNode->valuestring << "\n";
					mpModel->setProperty(MLSymbol(pNode->string), pNode->valuestring);
					break;
				case cJSON_Array:
					/*
					// TODO what is this doing here?! MLTEST
					if(mpAppView)
					{
						if(!strcmp(pNode->string, "window_bounds"))
						{
							assert(cJSON_GetArraySize(pNode) == 4);
							int x = cJSON_GetArrayItem(pNode, 0)->valueint;
							int y = cJSON_GetArrayItem(pNode, 1)->valueint;
							int w = cJSON_GetArrayItem(pNode, 2)->valueint;
							int h = cJSON_GetArrayItem(pNode, 3)->valueint;
							mpAppView->setPeerBounds(x, y, w, h);
						}
					}
					 */
					break;
				case cJSON_Object:
					// 	debug() << "looking at object: \n";
					// see if object is a stored signal
					cJSON* pObjType = cJSON_GetObjectItem(pNode, "type");
					if(pObjType && !strcmp(pObjType->valuestring, "signal") )
					{
						//debug() << " depth " << depth << " loading signal param " << pNode->string << "\n";
						MLSignal* pSig;
						int width = cJSON_GetObjectItem(pNode, "width")->valueint;
						int height = cJSON_GetObjectItem(pNode, "height")->valueint;
						int sigDepth = cJSON_GetObjectItem(pNode, "depth")->valueint;
						pSig = new MLSignal(width, height, sigDepth);
						if(pSig)
						{
							// read data into signal and set model param
							float* pSigData = pSig->getBuffer();
							int widthBits = bitsToContain(width);
							int heightBits = bitsToContain(height);
							int depthBits = bitsToContain(sigDepth);
							int size = 1 << widthBits << heightBits << depthBits;
							cJSON* pData = cJSON_GetObjectItem(pNode, "data");
							int dataSize = cJSON_GetArraySize(pData);
							if(dataSize == size)
							{
								// read array
								cJSON *c=pData->child;
								int i = 0;
								while (c)
								{
									pSigData[i++] = c->valuedouble;
									c=c->next;
								}
							}
							else
							{
								MLError() << "MLAppState::setStateFromJSON: wrong array size!\n";
							}
							mpModel->setProperty(MLSymbol(pNode->string), *pSig);
						}
					}
					
					break;
			}
		}
		
		if(pNode->child && depth < 1)
		{
			setStateFromJSON(pNode->child, depth + 1);
		}
		pNode = pNode->next;
	}
	
	updateAllProperties();
}

void MLAppState::loadDefaultState()
{
//	load defaultappstate_txt ?
}

File MLAppState::getAppStateDir() const
{
 	String makerName(File::createLegalFileName (String(mpMakerName)));
 	String applicationName(File::createLegalFileName (String(mpAppName)));

   #if JUCE_MAC || JUCE_IOS
    File dir ("~/Library/Application Support"); // user Library
    dir = dir.getChildFile (makerName);
    dir = dir.getChildFile (applicationName);

   #elif JUCE_LINUX || JUCE_ANDROID
	File dir ("~/" + "." + makerName + "/" + applicationName);

   #elif JUCE_WINDOWS
    File dir (File::getSpecialLocation (File::userApplicationDataDirectory));

    if (dir == File::nonexistent)
        return File::nonexistent;

    dir = dir.getChildFile(makerName);
    dir = dir.getChildFile(applicationName);
   #endif

	return dir;
}

File MLAppState::getAppStateFile() const
{
    String applicationName(mpAppName);
	String extension("txt");
	File dir(getAppStateDir());
    return dir.getChildFile(applicationName + "AppState").withFileExtension (extension);
}

void MLAppState::clearStateStack()
{
	mStateStack.clear();
}

void MLAppState::pushStateToStack()
{
	mStateStack.push_back(getStateAsBinary());
}

void MLAppState::popStateFromStack()
{
	if(mStateStack.size() > 0)
	{
		MemoryBlock p = mStateStack.back();
		mStateStack.pop_back();
		setStateFromBinary(p);
	}
}

void MLAppState::returnToFirstSavedState()
{
	MemoryBlock p;
	if(mStateStack.size() > 0)
	{
		p = mStateStack[0];
		mStateStack.clear();
		mStateStack.push_back(p);
		setStateFromBinary(p);
	}
}



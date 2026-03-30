#pragma once
#include "EIContext.h"
#include <UnigineXml.h>

bool save(const EIAction &v, const char *path);
bool save(const EIContextImpl &v, const char *path);

void save(const EIAction &v, const Unigine::XmlPtr &xml);
void save(const EIContextImpl &v, const Unigine::XmlPtr &xml);
void save(const EIActionMappings &v, const Unigine::XmlPtr &xml);
void save(const EIMapping &v, const Unigine::XmlPtr &xml);
void save(const EIKeyBinding &v, const Unigine::XmlPtr &xml);
void save(const EIModifier *v, const Unigine::XmlPtr &xml);
void save(const EITrigger *v, const Unigine::XmlPtr &xml);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool load(EIAction &v, const Unigine::XmlPtr &xml);
bool load(EIContextImpl &v, const Unigine::XmlPtr &xml);
void load(EIActionMappings &v, const Unigine::XmlPtr &xml);
void load(EIMapping &v, const Unigine::XmlPtr &xml);
void load(EIKeyBinding &v, const Unigine::XmlPtr &xml);
void load(EIModifier **v, const Unigine::XmlPtr &xml);
void load(EITrigger **v, const Unigine::XmlPtr &xml);

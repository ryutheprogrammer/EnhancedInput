#pragma once
#include "EIContext.h"
#include <UnigineXml.h>

bool save(const EIAction &v, const char *path);
bool save(const EIContextImpl &v, const char *path);

void save(const EIAction &v, const Unigine::XmlPtr &xml);
void save(const EIContextImpl &v, const Unigine::XmlPtr &xml);
void save(const EIKeyActionMapping *v, const Unigine::XmlPtr &xml);
void save(const EIModifier *v, const Unigine::XmlPtr &xml);
void save(const EITrigger *v, const Unigine::XmlPtr &xml);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool load(EIAction &v, const Unigine::XmlPtr &xml);
bool load(EIContextImpl &v, const Unigine::XmlPtr &xml);
void load(EIKeyActionMapping *v, const Unigine::XmlPtr &xml);
void load(EIModifier **v, const Unigine::XmlPtr &xml);
void load(EITrigger **v, const Unigine::XmlPtr &xml);

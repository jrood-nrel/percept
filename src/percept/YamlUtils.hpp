// Copyright 2014 Sandia Corporation. Under the terms of
// Contract DE-AC04-94AL85000 with Sandia Corporation, the
// U.S. Government retains certain rights in this software.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef percept_YamlUtils_hpp
#define percept_YamlUtils_hpp

#include <sstream>
#include <fstream>
#include <map>
#include <set>

#if STK_ADAPT_HAVE_YAML_CPP
#include <yaml-cpp/yaml.h>

#ifndef YAML_CHECK
#  define YAML_CHECK(emitter) do { if (1 && !emitter.good()) { std::cout << "Emitter error: " << __FILE__ << ":" << __LINE__ << " emitter.good()= " \
                                                                         << emitter.good() << " Error Message: " << emitter.GetLastError() << std::endl; return;} } while(0)
#endif

#ifndef YAML_ERRCHECK
#  define YAML_ERRCHECK YAML_CHECK(emitter)
#endif

#endif

  namespace percept {

#if STK_ADAPT_HAVE_YAML_CPP

    /// Set @param result if the @param key is present in the @param node, else set it to the given default value
    template<typename T>
    void set_if_present(const YAML::Node & node, const std::string& key, T& result, const T& default_if_not_present = T())
    {
      const YAML::Node *value = node.FindValue(key);
      if (value)
        *value >> result;
      else
        result = default_if_not_present;
    }

    /// this version doesn't change @param result unless the @param key is present in the @param node
    template<typename T>
    void set_if_present_no_default(const YAML::Node & node, const std::string& key, T& result)
    {
      const YAML::Node *value = node.FindValue(key);
      if (value)
        *value >> result;
    }

    struct YamlUtils {

      static void emit(YAML::Emitter& emout, const YAML::Node & node) {
        // recursive depth first
        YAML::NodeType::value type = node.Type();
        std::string out;
        switch (type)
          {
          case YAML::NodeType::Scalar:
            node >> out;
            emout << out;
            break;
          case YAML::NodeType::Sequence:
            emout << YAML::BeginSeq;
            for (unsigned int i = 0; i < node.size(); ++i) {
              const YAML::Node & subnode = node[i];
              emit(emout, subnode);
            }
            emout << YAML::EndSeq;
            break;
          case YAML::NodeType::Map:
            emout << YAML::BeginMap ;
            for (YAML::Iterator i = node.begin(); i != node.end(); ++i) {
              const YAML::Node & key   = i.first();
              const YAML::Node & value = i.second();
              key >> out;
              emout << YAML::Key << out;
              emout << YAML::Value;
              emit(emout, value);
            }
            emout << YAML::EndMap ;
            break;
          case YAML::NodeType::Null:
            emout << " (empty) ";
            break;
          default:
            std::cerr << "Warning: emit: unknown/unsupported node type" << std::endl;
            break;
          }
      }

      /// uses Emitter to print node to stream
      static void emit(std::ostream& sout, const YAML::Node & node) {
        YAML::Emitter out;
        emit(out,node);
        sout << out.c_str() << std::endl;
      }

    };


#endif
  }

#endif
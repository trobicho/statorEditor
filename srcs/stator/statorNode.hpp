#pragma once
#include <boost/json.hpp>
#include <functional>
#include <imgui.h>
#include <string>
#include "ImNodeFlow.h"
#include "stator.hpp"

enum StatorNodeType {
  SNT_NA,
  SNT_FACTORY_NODE,
  SNT_RECIPE_NODE,
  SNT_PART_NODE,
  SNT_IN_NODE,
  SNT_OUT_NODE,
};

StatorNodeType  sntFromString(std::string type);

namespace json = boost::json;
using namespace ImFlow;

struct  StatorNode : BaseNode {
  virtual void            drawPopUp() {;}
  virtual StatorNodeType  statorNodeType() = 0;
  virtual json::value     toJson() = 0;
  virtual void            fromJson(json::value value) {;}
};

struct  InputNode: public StatorNode {
  InputNode(double v = 0.0): value(v) {
    setTitle("Input");
    addOUT<double>("out")->behaviour([this](){return (value);});
  }

  void  draw() override {
    ImGui::SetNextItemWidth(100.f);
    ImGui::InputDouble("##Val", &value);
  }

  StatorNodeType  statorNodeType() override {
    return (SNT_IN_NODE);
  };

  json::value     toJson() override {
    json::object value = {
      {"type", "SNT_IN_NODE"},
      {"value", value},
    };
    return (value);
  }
  void            fromJson(json::value value) override {
    this->value = value.as_object()["value"].as_double();
  }

  double  value = 0.0;
};

struct  OutputNode: public StatorNode {
  OutputNode() {
    setTitle("Output");
    addIN<double>("in", 0, ConnectionFilter::SameType());
  }

  void  draw() override {
    double r = this->getInVal<double>("in");
    ImGui::SetNextItemWidth(100.f);
    ImGui::Text("%f", r);
  }

  StatorNodeType  statorNodeType() override {
    return (SNT_OUT_NODE);
  };

  json::value     toJson() override {
    json::object value = {
      {"type", "SNT_OUT_NODE"},
    };
    return (value);
  }
};

struct  PartNode: public StatorNode {
  PartNode(Part& a_part): part(a_part) {
    setTitle(part.name.c_str());
    setStyle(NodeStyle::red());

    addInPin();
    addOutPin();
  }
  ~PartNode() {
    for (auto& out: outRatios) {
      delete(out);
    }
  }

  struct  OutRatioPin {
    OutRatioPin(double r): ratio(r) {};

    double  calcQuantity(double quantity) {
      return (quantity * ratio);
    }

    double    ratio;
  };

  void  addInPin() {
    inCount += 1;
    std::string name = "in" + std::to_string(inCount);
    addIN<double>(name, 0, ConnectionFilter::SameType());
  }

  void  addOutPin(double r = 1.0) {
    OutRatioPin*  pin = new OutRatioPin(r);
    outRatios.push_back(pin);
    std::string name = "out" + std::to_string(outRatios.size());
    addOUT<double>(name)->behaviour([this, pin](){
      double quantity = 0.0;
      for (int i = 0; i < inCount; i++) {
        std::string name = "in" + std::to_string(i + 1);
        quantity += getInVal<double>(name);
      }
      return (pin->calcQuantity(quantity));
    });
  }
  void  removeIn() {
    if (inCount > 0) {
      std::string name = "in" + std::to_string(inCount);
      dropIN(name);
      inCount -= 1;
    }
  }
  void  removeOut() {
    if (outRatios.size() > 0) {
      std::string name = "out" + std::to_string(outRatios.size());
      dropOUT(name);
      delete(outRatios.back());
      outRatios.resize(outRatios.size() - 1);
    }
  }
  void  reset() {
    while (inCount > 0)
      removeIn();
    while (outRatios.size() > 0)
      removeOut();
  }

  void  draw() override {
    int i = 0;
    for (auto& out: outRatios) {
      ImGui::SetNextItemWidth(100.f);
      ImGui::PushID(i);
      ImGui::InputDouble("##out", &out->ratio);
      ImGui::PopID();
      i++;
    }
  }

  void  drawPopUp() override {
    ImGui::Separator();
    if (ImGui::Button("addIn"))
      addInPin();
    ImGui::SameLine();
    if (ImGui::Button("addOut"))
      addOutPin();
    if (ImGui::Button("removeIn")) {
      if (inCount > 1) {
        removeIn();
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("removeOut"))
      removeOut();
  }

  StatorNodeType  statorNodeType() override {
    return (SNT_PART_NODE);
  };

  json::value     toJson() override {
    json::array outs;
    for (auto& out: outRatios) {
      outs.push_back(out->ratio);
    }
    json::object value = {
      {"type", "SNT_PART_NODE"},
      {"name", part.name},
      {"inCount", inCount},
      {"outs", outs},
    };
    return (value);
  }

  void            fromJson(json::value value) override {
    reset();
    int inN = value.as_object()["inCount"].as_int64();
    while (inCount < inN) {
      addInPin();
    }
    json::array outs = value.as_object()["outs"].as_array();
    for (auto& out: outs) {
      addOutPin(out.as_double());
    }
  }

  int                       inCount = 0;
  Part&                     part;
  std::vector<OutRatioPin*> outRatios;
};


struct  RecipeNode: public StatorNode {
  RecipeNode(Recipe& a_recipe): recipe(a_recipe) {
    setTitle("Recipe");
    setStyle(NodeStyle::green());
    for (auto& in: recipe.inputs) {
      addIN<double>(in.name, 0, ConnectionFilter::SameType());
    }
    for (auto& out: recipe.outputs) {
      addOUT<double>(out.name)->behaviour([this, out](){
        double ratioMin = calcRatio();
        return (ratioMin * out.quantity);
      });
    }
  }

  inline double  calcRatio(PartWithQuantity& in) {return (getInVal<double>(in.name) / in.quantity);}
  double  calcRatio() {
    double ratioMin = calcRatio(recipe.inputs[0]);
    for (int i = 1; i < recipe.inputs.size(); i++) {
      double r = calcRatio(recipe.inputs[i]);
      if (ratioMin > r)
        ratioMin = r;
    }
    return (ratioMin);
  }

  void  drawPopUp() override {
    ImGui::Separator();
    recipe.drawPopUp();
    double  ratioMin = calcRatio();
    ImGui::Separator();
    ImGui::Text("Surplus:");
    for (auto& in: recipe.inputs) {
      double inVal = this->getInVal<double>(in.name);
      double r = inVal / in.quantity;
      if (r > ratioMin)
        ImGui::Text("%s: %lf", in.name.c_str(), inVal * (r - ratioMin));
    }
  }

  StatorNodeType  statorNodeType() override {
    return (SNT_RECIPE_NODE);
  };

  json::value     toJson() override {
    json::object value = {
      {"type", "SNT_RECIPE_NODE"},
      {"recipeId", recipe.id},
    };
    return (value);
  }

  Recipe&   recipe;
};

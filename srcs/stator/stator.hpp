#pragma once
#include <boost/json.hpp>
#include <functional>
#include <imgui.h>
#include <string>
#include "ImNodeFlow.h"

namespace json = boost::json;
using namespace ImFlow;

struct  PartWithQuantity {
  PartWithQuantity() {};
  PartWithQuantity(json::object part) {
    name = part["Part"].as_string();
    if (part["Quantity"].is_int64())
      quantity = (double)part["Quantity"].as_int64();
    else
      quantity = (double)part["Quantity"].as_double();
  }

  std::string   name;
  double        quantity;
};

struct  Recipe ;

struct  StatorNode : BaseNode {
  virtual void  drawPopUp() = 0;
};

struct  Part {
  Part(json::object partJson) {
    name = partJson["name"].as_string();
    imgPath = partJson["img"].as_string();
  }

  void  addRecipe(Recipe& recipe) {
    recipes.push_back(&recipe);
  }

  std::string           name;
  std::string           imgPath;
  std::vector<Recipe*>  recipes;
};

struct  Recipe {
  Recipe(json::object recipeJson) {
    for (auto& input: recipeJson["Input"].as_array()) {
      inputs.push_back(PartWithQuantity(input.as_object()));
    }
    for (auto& output: recipeJson["Output"].as_array()) {
      outputs.push_back(PartWithQuantity(output.as_object()));
    }
  }
  
  void  drawPopUp() {
    ImGui::Text("IN:");
    for (auto& in: inputs) {
      ImGui::Text("%s, %lf", in.name.c_str(), in.quantity);
    }
    ImGui::Text("OUT:");
    for (auto& out: outputs) {
      ImGui::Text("%s, %lf", out.name.c_str(), out.quantity);
    }
  }

  std::vector<PartWithQuantity>   inputs; 
  std::vector<PartWithQuantity>   outputs; 
};

struct  InputNode: public StatorNode {
  InputNode() {
    setTitle("Input");
    addOUT<double>("out")->behaviour([this](){return (value);});
  }

  void  draw() override {
    ImGui::SetNextItemWidth(100.f);
    ImGui::InputDouble("##Val", &value);
  }

  void  drawPopUp() {
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

  void  drawPopUp() {
  }
};

struct  PartNode: public StatorNode {
  PartNode(Part& a_part): part(a_part) {
    setTitle(part.name.c_str());
    setStyle(NodeStyle::red());

    addIN<double>("in1", 0, ConnectionFilter::SameType());
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

  void  addOutPin() {
    OutRatioPin*  pin = new OutRatioPin(1.0);
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

  void  drawPopUp() {
    if (ImGui::Button("addIn")) {
      inCount += 1;
      std::string name = "in" + std::to_string(inCount);
      addIN<double>(name, 0, ConnectionFilter::SameType());
    }
    if (ImGui::Button("addOut")) {
      addOutPin();
    }
    if (ImGui::Button("removeIn")) {
      if (inCount > 1) {
        std::string name = "in" + std::to_string(inCount);
        dropIN(name);
        inCount -= 1;
      }
    }
    if (ImGui::Button("removeOut")) {
      if (outRatios.size() > 0) {
        std::string name = "out" + std::to_string(outRatios.size());
        dropOUT(name);
        delete(outRatios.back());
        outRatios.resize(outRatios.size() - 1);
      }
    }
  }

  int                       inCount = 1;
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
        calcRatio();
        return (ratio * out.quantity);
      });
    }
  }

  void  drawPopUp() {
    recipe.drawPopUp();
    calcRatio();
    ImGui::Text("Surplus:");
    for (auto& in: recipe.inputs) {
      double inVal = this->getInVal<double>(in.name);
      double r = inVal / in.quantity;
      if (r > ratio)
        ImGui::Text("%s: %lf", in.name.c_str(), inVal * (r - ratio));
    }
  }

  void  calcRatio() {
    double ratioMin = -1.0;
    for (auto& in: recipe.inputs) {
      double r = this->getInVal<double>(in.name) / in.quantity;
      if (ratioMin < 0.0 || ratioMin > r)
        ratioMin = r;
    }
    ratio = ratioMin;
  }

  Recipe&   recipe;
  double    ratio = 1.0;
};

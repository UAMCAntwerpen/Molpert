#pragma once
#ifndef _PY_MOLECULAR_CONSTRAINTS_HPP_
#define _PY_MOLECULAR_CONSTRAINTS_HPP_

#include "MolecularConstraints.hpp"
#include <boost/python.hpp>

namespace python = boost::python;

// Credit for the converter voodoo goes to Tanner Sansbury @ StackOverflow.
struct FunctionConverter {
  // Registers converter from a python callable type to the provided type.
  template <class FunctionSignature>
  FunctionConverter& Register() {
    python::converter::registry::push_back(
      &FunctionConverter::Convertible,
      &FunctionConverter::Construct<FunctionSignature>,
      python::type_id<std::function<FunctionSignature>>());
    return *this; // Support chaining.
  };

  // Check if PyObject is callable.
  static void* Convertible(PyObject* object) {
    return PyCallable_Check(object) ? object : NULL;
  };

  // Convert callable PyObject to a C++ std::function.
  template <class FunctionSignature>
  static void Construct(
    PyObject* object,
    python::converter::rvalue_from_python_stage1_data* data) {
    // Object is a borrowed reference, so create a handle indicting it is
    // borrowed for proper reference counting.
    python::handle<> handle (python::borrowed(object));
    // Obtain a handle to the memory block that the converter has allocated
    // for the C++ type.
    typedef std::function<FunctionSignature> Function;
    typedef python::converter::rvalue_from_python_storage<Function> Storage;
    void* storage = reinterpret_cast<Storage*>(data)->storage.bytes;
    // Allocate the C++ type into the converter's memory block, and assign
    // its handle to the converter's convertible variable.
    new (storage) Function(python::object(handle));
    data->convertible = storage;
  };
};

// The RDKit only registered a RDKit::ROMol* pointer to-python converter,
// which corresponds to the rdchem.Mol class. All functions expecting a
// rdchem.Mol, including potential MoleculeConstraints, are actually expecting 
// a RDKit::ROMol*. We can register a shim RDKit::ROMol to rdchem.Mol converter.
struct ROMolConverter {
  static PyObject* convert(const RDKit::ROMol& molecule) {
    boost::python::object object (boost::python::ptr(&molecule));
    return boost::python::incref(object.ptr());
  };
};

void WrapMolecularConstraints() {
  FunctionConverter()
    .Register<bool(const RDKit::Atom*)>()
    .Register<bool(const RDKit::Bond*)>()
    .Register<bool(const RDKit::ROMol&)>()
    .Register<std::optional<MolecularConstraints::AtomConstraint>(
      const RDKit::Atom*, const RDKit::Atom*)>()
    .Register<std::optional<MolecularConstraints::BondConstraint>(
      const RDKit::Bond*, const RDKit::Bond*)>();

  python::to_python_converter<RDKit::ROMol, ROMolConverter>();

  python::class_ constraints = python::class_<MolecularConstraints>(
    "MolecularConstraints", python::init())
  .def(python::init<
    const MolecularConstraints::AtomConstraintGenerator&>((
    python::arg("atom_constraint_generator"))))
  .def(python::init<
    const MolecularConstraints::BondConstraintGenerator&>((
    python::arg("bond_constraint_generator"))))
  .def(python::init<
    const MolecularConstraints::AtomConstraintGenerator&,
    const MolecularConstraints::BondConstraintGenerator&>((
    python::arg("atom_constraint_generator"),
    python::arg("bond_constraint_generator"))))
  .def("SetAtomConstraint", &MolecularConstraints::SetAtomConstraint, (
    python::arg("atom_tag"), 
    python::arg("atom_constraint"), 
    python::arg("replace") = true,
    python::arg("make_static") = false))
  .def("SetBondConstraint", &MolecularConstraints::SetBondConstraint, (
    python::arg("bond_tag"), 
    python::arg("bond_constraint"), 
    python::arg("replace") = true,
    python::arg("make_static") = false))
  .def("SetMoleculeConstraint", &MolecularConstraints::SetMoleculeConstraint, (
    python::arg("molecule_constraint"), 
    python::arg("make_static") = false))
  .def("GenerateAtomConstraint", &MolecularConstraints::GenerateAtomConstraint, (
    python::arg("atom")))
  .def("GenerateAtomConstraints", &MolecularConstraints::GenerateAtomConstraints, (
    python::arg("molecule")))
  .def("GenerateBondConstraint", &MolecularConstraints::GenerateBondConstraint, (
    python::arg("bond")))
  .def("GenerateBondConstraints", &MolecularConstraints::GenerateBondConstraints, (
    python::arg("molecule")))
  .def("GenerateConstraints", &MolecularConstraints::GenerateConstraints, (
    python::arg("molecule")))
  .def("UpdateAtomConstraint", &MolecularConstraints::UpdateAtomConstraint, (
    python::arg("atom_tag"),
    python::arg("prior_atom"),
    python::arg("posterior_atom")))
  .def("UpdateBondConstraint", &MolecularConstraints::UpdateBondConstraint, (
    python::arg("bond_tag"),
    python::arg("prior_bond"),
    python::arg("posterior_bond")))
  .def("UpdateConstraints", &MolecularConstraints::UpdateConstraints, (
    python::arg("molecule"),
    python::arg("perturbation")))
  .def("ClearAtomConstraint", &MolecularConstraints::ClearAtomConstraint, (
    python::arg("atom_tag"),
    python::arg("clear_static") = false))
  .def("ClearBondConstraint", &MolecularConstraints::ClearBondConstraint, (
    python::arg("bond_tag"),
    python::arg("clear_static") = false))
  .def("ClearMoleculeConstraint", &MolecularConstraints::ClearMoleculeConstraint, (
    python::arg("constraint_idx"),
    python::arg("clear_static") = false))
  .def("ClearAtomConstraints", &MolecularConstraints::ClearAtomConstraints, (
    python::arg("clear_static") = false))
  .def("ClearBondConstraints", &MolecularConstraints::ClearBondConstraints, (
    python::arg("clear_static") = false))
  .def("ClearMoleculeConstraints", &MolecularConstraints::ClearMoleculeConstraints, (
    python::arg("clear_static") = false))
  .def("ClearConstraints", &MolecularConstraints::ClearConstraints, (
    python::arg("clear_static") = false))
  .def("Clear", &MolecularConstraints::Clear, (
    python::arg("clear_static") = false))
  .def<bool (MolecularConstraints::*)(Tag, const RDKit::Atom*) const>(
    "IsAtomAllowed", &MolecularConstraints::IsAllowed, (
    python::arg("atom_tag"),
    python::arg("atom")))
  .def<bool (MolecularConstraints::*)(Tag, const RDKit::Bond*) const>(
    "IsBondAllowed", &MolecularConstraints::IsAllowed, (
    python::arg("bond_tag"),
    python::arg("bond")))
  .def<bool (MolecularConstraints::*)(Tag, const RDKit::Atom*, const RDKit::Atom*) const>(
    "IsAtomChangeAllowed", &MolecularConstraints::IsAllowed, (
    python::arg("atom_tag"),
    python::arg("prior_atom"),
    python::arg("posterior_atom")))
  .def<bool (MolecularConstraints::*)(Tag, const RDKit::Bond*, const RDKit::Bond*) const>(
    "IsBondChangeAllowed", &MolecularConstraints::IsAllowed, (
    python::arg("bond_tag"),
    python::arg("prior_bond"),
    python::arg("posterior_bond")))
  .def<bool (MolecularConstraints::*)(const RDKit::ROMol&, const MolecularPerturbation&) const>(
    "IsPerturbationAllowed", &MolecularConstraints::IsAllowed, (
    python::arg("molecule"),
    python::arg("perturbation")))
  .def<bool (MolecularConstraints::*)(Tag, const RDKit::Atom*, const RDKit::Atom*)>(
    "UpdateAtomConstraintIfAllowed", &MolecularConstraints::UpdateIfAllowed, (
    python::arg("atom_tag"),
    python::arg("prior_atom"),
    python::arg("posterior_atom")))
  .def<bool (MolecularConstraints::*)(Tag, const RDKit::Bond*, const RDKit::Bond*)>(
    "UpdateBondConstraintIfAllowed", &MolecularConstraints::UpdateIfAllowed, (
    python::arg("bond_tag"),
    python::arg("prior_bond"),
    python::arg("posterior_bond")))
  .def<bool (MolecularConstraints::*)(const RDKit::ROMol&, const MolecularPerturbation&)>(
    "UpdateConstraintsIfAllowed", &MolecularConstraints::UpdateIfAllowed, (
    python::arg("molecule"),
    python::arg("perturbation")))
  .def("HasAtomConstraintGenerator", &MolecularConstraints::HasAtomConstraintGenerator)
  .def("HasBondConstraintGenerator", &MolecularConstraints::HasBondConstraintGenerator)
  .def("HasConstraintGenerators", &MolecularConstraints::HasConstraintGenerators)
  .def("__bool__", &MolecularConstraints::operator bool)
  .def("__len__", &MolecularConstraints::Size);
};

#endif // !_PY_MOLECULAR_CONSTRAINTS_HPP_
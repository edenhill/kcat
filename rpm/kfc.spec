Name:     kfc
Version:  0.11.0
Release:  0%{?dist}
Summary:  kfc is a generic non-JVM producer and consumer for Apache Kafka 0.8, think of it as a netcat for Kafka.
Group:    Productivity/Networking/Other
License:  BSD-2-Clause
URL:      https://github.com/fsaintjacques/kfc
Source:   kfc-%{version}.tar.gz
Requires: librdkafka1

BuildRequires: zlib-devel gcc >= 4.1 librdkafka-devel
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description
kfc is a generic non-JVM producer and consumer for Apache Kafka 0.8,
think of it as a netcat for Kafka.

In producer mode kfc reads messages from stdin, delimited with a
configurable delimeter (-d, defaults to newline), and produces them to the
provided Kafka cluster (-b), topic and partition (-p).

In consumer mode kfc reads messages from a topic and partition and prints
them to stdout using the configured message delimiter.

kfc also features a Metadata list  mode to display the current state
of the Kafka cluster and its topics and partitions.

kfc is fast and lightweight; statically linked it is no more than 150Kb.

%prep
%setup -q

%configure

%build
make

%install
rm -rf %{buildroot}
DESTDIR=%{buildroot} make install

%clean
rm -rf %{buildroot}

%files -n %{name}
%defattr(755,root,root)
%{_bindir}/kfc
%defattr(644,root,root)
%doc README.md
%doc LICENSE
%doc doc/kfc.1

%changelog
* Thu Jan 15 2015 François Saint-Jacques <fsaintjacques@gmail.com> 0.11.0-0
- Adds manpage
* Tue Jan 14 2015 François Saint-Jacques <fsaintjacques@gmail.com> 0.10.0-0
- Initial RPM package

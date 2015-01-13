Name:     kc
Version:  0.9.1
Release:  0%{?dist}
Summary:  kc is a generic non-JVM producer and consumer for Apache Kafka 0.8, think of it as a netcat for Kafka.
Group:    Productivity/Networking/Other
License:  BSD-2-Clause
URL:      https://github.com/fsaintjacques/kc
Source:   kc-%{version}.tar.gz
Requires: librdkafka1

BuildRequires: zlib-devel gcc >= 4.1 librdkafka-devel
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description
kc is a generic non-JVM producer and consumer for Apache Kafka 0.8,
think of it as a netcat for Kafka.

In producer mode kc reads messages from stdin, delimited with a
configurable delimeter (-d, defaults to newline), and produces them to the
provided Kafka cluster (-b), topic and partition (-p).

In consumer mode kc reads messages from a topic and partition and prints
them to stdout using the configured message delimiter.

kc also features a Metadata list  mode to display the current state
of the Kafka cluster and its topics and partitions.

kc is fast and lightweight; statically linked it is no more than 150Kb.

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
%{_bindir}/kc
%defattr(644,root,root)
%doc README.md
%doc LICENSE
%doc doc/kc.1

%changelog
* Tue Jan 14 2015 François Saint-Jacques <fsaintjacques@gmail.com> 0.9.1-0
- Initial RPM package
